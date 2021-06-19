#include <ext/checksum.h>
#include <ext/gzip.h>
#include <ext/mapped_file.h>
#include <stdio.h>
#include <sys/mman.h>

namespace Ext {

enum GZipFlags {
    FTEXT = 1,
    FHCRC = 2,
    FEXTRA = 4,
    FNAME = 8,
    FCOMMENT = 16,
};

GZipEncoder::GZipEncoder(ByteWriter& writer) : m_encoder(encode()), m_writer(writer), m_deflate_encoder(writer) {}

GZipEncoder ::~GZipEncoder() {}

StreamResult GZipEncoder::stream_data(Span<const uint8_t> input, StreamFlushMode mode) {
    m_input = input;
    m_flush_mode = mode;
    return m_encoder();
}

Generator<StreamResult> GZipEncoder::write_bytes(const uint8_t* bytes, size_t byte_count) {
    for (size_t i = 0; i < byte_count;) {
        if (!m_writer.write_byte(bytes[i])) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> GZipEncoder::encode() {
    struct [[gnu::packed]] GZipHeader {
        uint8_t id1;
        uint8_t id2;
        uint8_t compression_method;
        uint8_t flags;
        uint32_t time_last_modified;
        uint8_t extra_flags;
        uint8_t os_field;
    };

    uint8_t flags = (m_gzip_data.comment.has_value() ? GZipFlags::FCOMMENT : 0) | (m_gzip_data.name.has_value() ? GZipFlags::FNAME : 0);
    GZipHeader header = {
        .id1 = 0x1F,
        .id2 = 0x8B,
        .compression_method = 8,
        .flags = flags,
        .time_last_modified = (uint32_t) m_gzip_data.time_last_modified,
        .extra_flags = 0,
        .os_field = 3,
    };

    co_yield write_bytes((const uint8_t*) &header, sizeof(header));

    if (flags & GZipFlags::FNAME) {
        co_yield write_bytes((const uint8_t*) m_gzip_data.name.value().string(), m_gzip_data.name.value().size() + 1);
    }

    if (flags & GZipFlags::FCOMMENT) {
        co_yield write_bytes((const uint8_t*) m_gzip_data.comment.value().string(), m_gzip_data.comment.value().size() + 1);
    }

    uint32_t crc32 = 0;
    uint32_t total_size = 0;

    for (;;) {
        total_size += m_input.size();
        crc32 = compute_partial_crc32_checksum(m_input.data(), m_input.size(), crc32);

        auto result = m_deflate_encoder.stream_data(m_input, m_flush_mode);
        if (result == StreamResult::Error) {
            co_yield result;
            co_return;
        }

        if (result == StreamResult::NeedsMoreInput) {
            co_yield result;
            continue;
        }

        break;
    }

    co_yield write_bytes((const uint8_t*) &crc32, sizeof(crc32));
    co_yield write_bytes((const uint8_t*) &total_size, sizeof(total_size));

    co_yield StreamResult::Success;
}

Maybe<GZipData> read_gzip_path(const String& path) {
    auto file_mapping = MappedFile::create(path, PROT_READ, MAP_SHARED);
    if (!file_mapping) {
        return {};
    }

    size_t offset = 0;
    auto* raw_data = file_mapping->data();
    if (raw_data[offset++] != 0x1F || raw_data[offset++] != 0x8B) {
        fprintf(stderr, "`%s' not gzip file\n", path.string());
        return {};
    }

    uint8_t compression_method = raw_data[offset++];
    if (compression_method != 8) {
        fprintf(stderr, "`%s' has unsupported compression method: %u\n", path.string(), compression_method);
        return {};
    }

    uint8_t flags = raw_data[offset++];
    time_t time_last_modified = *((uint32_t*) &raw_data[offset]);
    offset += sizeof(uint32_t);

    uint8_t extended_flags = raw_data[offset++];
    (void) extended_flags;

    uint8_t os = raw_data[offset++];
    (void) os;

    Maybe<Vector<uint8_t>> extra_data;
    if (flags & GZipFlags::FEXTRA) {
        uint16_t extra_length = *((uint16_t*) &raw_data[offset]);
        offset += sizeof(uint16_t);
        // FIXME: why doesn't this work?
        // extra_data = Vector<uint8_t>(&raw_data[offset], extra_length);
        offset += extra_length;
    }

    Maybe<String> original_name;
    if (flags & GZipFlags::FNAME) {
        original_name = String((char*) &raw_data[offset]);
        offset += original_name.value().size() + 1;
    }

    Maybe<String> comment;
    if (flags & GZipFlags::FCOMMENT) {
        comment = String((char*) &raw_data[offset]);
        offset += comment.value().size() + 1;
    }

    Maybe<uint16_t> crc16;
    if (flags & GZipFlags::FHCRC) {
        crc16 = *((uint16_t*) &raw_data[offset]);
        offset += sizeof(uint16_t);
    }

    uint32_t crc32 = *((uint32_t*) &raw_data[file_mapping->size() - 2 * sizeof(uint32_t)]);
    (void) crc32;
    uint32_t isize = *((uint32_t*) &raw_data[file_mapping->size() - sizeof(uint32_t)]);
    (void) isize;

    uint8_t* compressed_data_start = &raw_data[offset];
    size_t compressed_data_length = file_mapping->size() - offset - 2 * sizeof(uint32_t);

    DeflateDecoder decoder;
    auto result = decoder.stream_data(compressed_data_start, compressed_data_length);
    if (result != StreamResult::Success) {
        return {};
    }
    GZipData data;
    data.comment = move(comment);
    data.decompressed_data = move(decoder.decompressed_data());
    data.extra_data = move(extra_data);
    data.name = move(original_name);
    data.time_last_modified = time_last_modified;
    return move(data);
}

}
