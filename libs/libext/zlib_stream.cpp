#include <arpa/inet.h>
#include <ext/checksum.h>
#include <ext/zlib_stream.h>

namespace Ext {
ZLibStreamDecoder::ZLibStreamDecoder() : StreamDecoder(decode()) {}

ZLibStreamDecoder::~ZLibStreamDecoder() {}

Generator<StreamResult> ZLibStreamDecoder::decode() {
    uint8_t compression_method;
    co_yield read_bytes(as_writable_bytes(compression_method));

    if ((compression_method & 0x0F) != 0x08) {
        co_yield StreamResult::Error;
        co_return;
    }

    if ((compression_method & 0xF0) > 0x70) {
        co_yield StreamResult::Error;
        co_return;
    }

    uint8_t flags;
    co_yield read_bytes(as_writable_bytes(flags));

    if ((((compression_method * 256U) + flags) % 31) != 0) {
        co_yield StreamResult::Error;
        co_return;
    }

    if (flags & 0x20U) {
        uint32_t dict;
        co_yield read_bytes(as_writable_bytes(dict));
        (void) dict;
    }

    uint32_t computed_adler32 = CHECKSUM_ADLER32_INIT;
    for (;;) {
        bool resume = false;
        for (;;) {
            m_deflate_decoder.set_output(writer().span_available());
            auto result = resume ? m_deflate_decoder.resume() : m_deflate_decoder.stream_data(reader().span_remaining());
            writer().advance(m_deflate_decoder.writer().bytes_written());
            if (result == StreamResult::Error) {
                co_yield result;
                co_return;
            }

            computed_adler32 = compute_partial_adler32_checksum(m_deflate_decoder.writer().data(),
                                                                m_deflate_decoder.writer().bytes_written(), computed_adler32);

            reader().advance(m_deflate_decoder.reader().byte_offset());

            if (result == StreamResult::NeedsMoreInput) {
                co_yield result;
                break;
            }

            if (result == StreamResult::NeedsMoreOutputSpace) {
                co_yield result;
                resume = true;
                m_deflate_decoder.reader().set_data(m_deflate_decoder.reader().span_remaining());
                continue;
            }
            goto finished;
        }
    }

finished:
    uint32_t expected_adler32;
    co_yield read_bytes(as_writable_bytes(expected_adler32));
    expected_adler32 = htonl(expected_adler32);
    if (expected_adler32 != computed_adler32) {
        co_yield StreamResult::Error;
        co_return;
    }

    co_yield StreamResult::Success;
}
}
