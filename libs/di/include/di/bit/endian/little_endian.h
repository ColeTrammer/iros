#pragma once

#include <di/bit/endian/endian.h>
#include <di/bit/operation/byteswap.h>
#include <di/format/prelude.h>

namespace di::bit {
template<concepts::Integral T>
class [[gnu::packed]] LittleEndian {
public:
    LittleEndian() = default;

    constexpr LittleEndian(T value) { *this = value; }

    constexpr LittleEndian operator=(T value) {
        if constexpr (Endian::Native == Endian::Little) {
            m_value = value;
        } else {
            m_value = byteswap(value);
        }
        return *this;
    }

    constexpr operator T() const {
        if constexpr (Endian::Native == Endian::Little) {
            return m_value;
        } else {
            return byteswap(m_value);
        }
    }

private:
    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<LittleEndian>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<T, Enc>(parse_context, debug);
    }

    T m_value { 0 };
};
}