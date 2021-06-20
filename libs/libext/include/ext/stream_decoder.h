#pragma once

#include <ext/stream.h>

namespace Ext {
class StreamDecoder : public Stream {
public:
    StreamDecoder(Generator<StreamResult> decoder);
    virtual ~StreamDecoder() override;

    StreamResult stream_data(Span<const uint8_t> input);
    StreamResult resume();

private:
    Generator<StreamResult> m_decoder;
};
}
