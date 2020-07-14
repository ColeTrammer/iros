#include <ext/deflate.h>

namespace Ext {

StreamResult ZLibStreamDecoder::stream_data(uint8_t* data, size_t data_len) {
    auto get = [&](uint8_t bytes) -> Maybe<uint32_t> {
        if (!m_in_get) {
            m_in_get = true;
            m_bytes_to_get = bytes;
            m_get_buffer = 0;
        }

        for (; m_offset < data_len && m_bytes_to_get; m_offset++) {
            uint8_t byte = data[m_offset];
            // NOTE: This puts multi byte numbers in network byte order.
            m_get_buffer |= (byte << (--m_bytes_to_get) * 8U);
        }

        if (m_bytes_to_get) {
            return {};
        }

        m_in_get = false;
        return { m_get_buffer };
    };

    m_offset = 0;
    switch (m_state) {
        case State::OnCMF:
            goto GetCMF;
        case State::OnFLG:
            goto GetFLG;
        case State::OnDICTID:
            goto GetDICTID;
        case State::OnDeflateData:
            goto GetDeflateData;
        case State::OnAdler32:
            goto GetAdler32;
    }

GetCMF : {
    m_state = State::OnCMF;
    auto cmf = get(1);
    if (!cmf.has_value()) {
        return StreamResult::NeedsMoreData;
    }
    m_compression_method = cmf.value();

    if ((m_compression_method & 0x0F) != 0x08) {
        return StreamResult::Error;
    }

    if ((m_compression_method & 0xF0) != 0x70) {
        return StreamResult::Error;
    }
}

GetFLG : {
    m_state = State::OnFLG;
    auto flg = get(1);
    if (!flg.has_value()) {
        return StreamResult::NeedsMoreData;
    }
    m_flags = flg.value();
}

    // FIXME: Check the FCHECK field
    if (m_flags & 0x10U) {
        goto GetDICTID;
    }
    goto GetDeflateData;

GetDICTID : {
    m_state = State::OnDICTID;
    auto dictid = get(4);
    if (!dictid.has_value()) {
        return StreamResult::NeedsMoreData;
    }
}

GetDeflateData : {
    m_state = State::OnDeflateData;
    auto result = m_deflate_stream.stream_data(data + m_offset, data_len - m_offset);
    if (result != StreamResult::Success) {
        return result;
    }
    m_offset += m_deflate_stream.last_byte_offset();
}

GetAdler32 : {
    m_state = State::OnAdler32;
    auto adler32 = get(4);
    if (!adler32.has_value()) {
        return StreamResult::NeedsMoreData;
    }

    // FIXME: Check the ADLER32 value
    return StreamResult::Success;
}
}

}
