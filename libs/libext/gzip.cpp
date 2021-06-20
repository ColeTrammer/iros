#include <ext/checksum.h>
#include <ext/gzip.h>
#include <ext/mapped_file.h>
#include <stdio.h>
#include <sys/mman.h>

namespace Ext {
constexpr uint8_t GZIP_ID1 = 0x1F;
constexpr uint8_t GZIP_ID2 = 0x8B;
constexpr uint8_t GZIP_COMPRESSION_METHOD_DEFLATE = 8;
constexpr uint8_t GZIP_OS_UNIX = 0x03;

struct [[gnu::packed]] GZipHeader {
    uint8_t id1;
    uint8_t id2;
    uint8_t compression_method;
    uint8_t flags;
    uint32_t time_last_modified;
    uint8_t extra_flags;
    uint8_t os_field;
};

enum GZipFlags {
    FTEXT = 1,
    FHCRC = 2,
    FEXTRA = 4,
    FNAME = 8,
    FCOMMENT = 16,
};

GZipDecoder::GZipDecoder() : m_decoder(decode()) {}

GZipDecoder::~GZipDecoder() {}

StreamResult GZipDecoder::stream_data(Span<const uint8_t> input) {
    m_reader.set_data(input);
    return resume();
}

StreamResult GZipDecoder::resume() {
    return m_decoder();
}

Generator<StreamResult> GZipDecoder::decode() {
    GZipHeader header;
    co_yield read_bytes(as_writable_bytes(header));

    if (header.id1 != GZIP_ID1 || header.id2 != GZIP_ID2 || header.compression_method != GZIP_COMPRESSION_METHOD_DEFLATE) {
        co_yield StreamResult::Error;
        co_return;
    }

    m_member_data.time_last_modified = header.time_last_modified;

    if (header.flags & GZipFlags::FEXTRA) {
        uint16_t extra_data_length;
        co_yield read_bytes(as_writable_bytes(extra_data_length));

        ByteBuffer extra_data_buffer(extra_data_length);
        extra_data_buffer.set_size(extra_data_length);
        co_yield read_bytes(extra_data_buffer.span());
        m_member_data.extra_data = move(extra_data_buffer);
    }

    if (header.flags & GZipFlags::FNAME) {
        String name;
        co_yield read_string(name);
        m_member_data.name = move(name);
    }

    if (header.flags & GZipFlags::FCOMMENT) {
        String comment;
        co_yield read_string(comment);
        m_member_data.comment = move(comment);
    }

    if (header.flags & GZipFlags::FHCRC) {
        uint16_t header_crc16;
        co_yield read_bytes(as_writable_bytes(header_crc16));
        (void) header_crc16;
    }

    uint32_t computed_crc32 = 0;
    uint32_t computed_total_size = 0;
    for (;;) {
        auto result = m_deflate_decoder.stream_data((uint8_t*) m_reader.data() + m_reader.byte_offset(), m_reader.bytes_remaining());
        if (result == StreamResult::Error) {
            co_yield result;
            co_return;
        }

        m_reader.advance(m_deflate_decoder.last_byte_offset());
        if (result == StreamResult::NeedsMoreInput || result == StreamResult::NeedsMoreOutputSpace) {
            co_yield result;
            continue;
        }

        if (result == StreamResult::Success) {
            computed_crc32 = compute_partial_crc32_checksum(m_deflate_decoder.decompressed_data().vector(),
                                                            m_deflate_decoder.decompressed_data().size(), computed_crc32);
            computed_total_size = m_deflate_decoder.decompressed_data().size();
            break;
        }
    }

    uint32_t expected_crc32;
    co_yield read_bytes(as_writable_bytes(expected_crc32));

    uint32_t expected_total_size;
    co_yield read_bytes(as_writable_bytes(expected_total_size));

    if (expected_crc32 != computed_crc32 || expected_total_size != computed_total_size) {
        co_yield StreamResult::Error;
        co_return;
    }

    co_yield write_bytes(m_deflate_decoder.decompressed_data().span());
    co_yield StreamResult::Success;
}

Generator<StreamResult> GZipDecoder::write_bytes(Span<const uint8_t> bytes) {
    for (size_t i = 0; i < bytes.size();) {
        if (!m_writer.write_byte(bytes[i])) {
            co_yield StreamResult::NeedsMoreOutputSpace;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> GZipDecoder::read_bytes(Span<uint8_t> bytes) {
    for (size_t i = 0; i < bytes.size();) {
        auto maybe_byte = m_reader.next_byte();
        if (!maybe_byte) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        bytes[i++] = maybe_byte.value();
    }
}

Generator<StreamResult> GZipDecoder::read_string(String& string) {
    for (;;) {
        auto maybe_byte = m_reader.next_byte();
        if (!maybe_byte) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        auto byte = maybe_byte.value();
        if (byte == '\0') {
            break;
        }

        string += String(byte);
    }
}

GZipEncoder::GZipEncoder() : m_encoder(encode()) {}

GZipEncoder::~GZipEncoder() {}

StreamResult GZipEncoder::stream_data(Span<const uint8_t> input, StreamFlushMode mode) {
    m_input = input;
    m_flush_mode = mode;
    return resume();
}

StreamResult GZipEncoder::resume() {
    return m_encoder();
}

Generator<StreamResult> GZipEncoder::write_bytes(Span<const uint8_t> bytes) {
    for (size_t i = 0; i < bytes.size();) {
        if (!m_writer.write_byte(bytes[i])) {
            co_yield StreamResult::NeedsMoreOutputSpace;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> GZipEncoder::encode() {
    uint8_t flags = (m_gzip_data.comment.has_value() ? GZipFlags::FCOMMENT : 0) | (m_gzip_data.name.has_value() ? GZipFlags::FNAME : 0);
    GZipHeader header = {
        .id1 = GZIP_ID1,
        .id2 = GZIP_ID2,
        .compression_method = GZIP_COMPRESSION_METHOD_DEFLATE,
        .flags = flags,
        .time_last_modified = (uint32_t) m_gzip_data.time_last_modified,
        .extra_flags = 0,
        .os_field = GZIP_OS_UNIX,
    };

    co_yield write_bytes(as_readonly_bytes(header));

    if (flags & GZipFlags::FNAME) {
        co_yield write_bytes({ (const uint8_t*) m_gzip_data.name.value().string(), m_gzip_data.name.value().size() + 1 });
    }

    if (flags & GZipFlags::FCOMMENT) {
        co_yield write_bytes({ (const uint8_t*) m_gzip_data.comment.value().string(), m_gzip_data.comment.value().size() + 1 });
    }

    uint32_t crc32 = 0;
    uint32_t total_size = 0;

    for (;;) {
        total_size += m_input.size();
        crc32 = compute_partial_crc32_checksum(m_input.data(), m_input.size(), crc32);

        for (;;) {
            m_deflate_encoder.set_output(m_writer.span_available());
            auto result = m_deflate_encoder.stream_data(m_input, m_flush_mode);
            m_writer.advance(m_deflate_encoder.writer().bytes_written());
            switch (result) {
                case StreamResult::Error:
                    co_yield result;
                    co_return;
                case StreamResult::NeedsMoreOutputSpace:
                    co_yield result;
                    continue;
                case StreamResult::NeedsMoreInput:
                    co_yield result;
                    break;
                case StreamResult::Success:
                    goto finish;
            }
            break;
        }
    }

finish:
    co_yield write_bytes(as_readonly_bytes(crc32));
    co_yield write_bytes(as_readonly_bytes(total_size));

    co_yield StreamResult::Success;
}
}
