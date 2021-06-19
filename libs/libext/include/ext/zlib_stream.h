#pragma once

#include <ext/deflate.h>

namespace Ext {
class ZLibStreamDecoder {
public:
    StreamResult stream_data(uint8_t* data, size_t data_len);

    Vector<uint8_t>& decompressed_data() { return m_deflate_stream.decompressed_data(); }
    const Vector<uint8_t>& decompressed_data() const { return m_deflate_stream.decompressed_data(); }

private:
    enum class State {
        OnCMF,
        OnFLG,
        OnDICTID,
        OnDeflateData,
        OnAdler32,
    };

    DeflateDecoder m_deflate_stream;
    State m_state { State::OnCMF };
    size_t m_offset { 0 };
    uint8_t m_compression_method { 0 };
    uint8_t m_flags { 0 };
    bool m_in_get { false };
    uint8_t m_bytes_to_get { 0 };
    uint32_t m_get_buffer { 0 };
};
}
