#pragma once

#include <ext/stream.h>

namespace Ext {
class StreamEncoder : public Stream {
public:
    StreamEncoder(Generator<StreamResult> encoder);
    virtual ~StreamEncoder() override;

    StreamFlushMode flush_mode() const { return m_flush_mode; }

    StreamResult stream_data(Span<const uint8_t> input, StreamFlushMode flush_mode);
    StreamResult resume();

private:
    Generator<StreamResult> m_encoder;
    StreamFlushMode m_flush_mode;
};
}
