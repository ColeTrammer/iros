#include <ext/stream_encoder.h>

namespace Ext {
StreamEncoder::StreamEncoder(Generator<StreamResult> encoder) : m_encoder(move(encoder)) {}

StreamEncoder::~StreamEncoder() {}

StreamResult StreamEncoder::stream_data(Span<const uint8_t> input, StreamFlushMode flush_mode) {
    m_flush_mode = flush_mode;
    reader().set_data(input);
    return resume();
}

StreamResult StreamEncoder::resume() {
    return m_encoder();
}
}
