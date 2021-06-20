#include <ext/stream_decoder.h>

namespace Ext {
StreamDecoder::StreamDecoder(Generator<StreamResult> decoder) : m_decoder(move(decoder)) {}

StreamDecoder::~StreamDecoder() {}

StreamResult StreamDecoder::stream_data(Span<const uint8_t> input) {
    reader().set_data(input);
    return resume();
}

StreamResult StreamDecoder::resume() {
    return m_decoder();
}
}
