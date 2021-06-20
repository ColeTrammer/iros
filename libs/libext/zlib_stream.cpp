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

    // FIXME: Check the FCHECK field

    if (flags & 0x20U) {
        uint32_t dict;
        co_yield read_bytes(as_writable_bytes(dict));
        (void) dict;
    }

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
    uint32_t adler32;
    co_yield read_bytes(as_writable_bytes(adler32));
    (void) adler32;

    co_yield StreamResult::Success;
}
}
