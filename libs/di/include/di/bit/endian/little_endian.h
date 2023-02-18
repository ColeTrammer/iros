#pragma once

#include <di/bit/endian/endian.h>
#include <di/bit/operation/byteswap.h>
#include <di/format/prelude.h>

namespace di::bit {
namespace detail {
    struct HostToLittleEndianFunction {
        template<concepts::Integral T>
        constexpr T operator()(T value) const {
            if constexpr (Endian::Native == Endian::Little) {
                return value;
            } else {
                return byteswap(value);
            }
        }
    };
}

constexpr inline auto host_to_little_endian = detail::HostToLittleEndianFunction {};
constexpr inline auto little_endian_to_host = detail::HostToLittleEndianFunction {};

template<concepts::Integral T>
class [[gnu::packed]] LittleEndian {
public:
    LittleEndian() = default;

    constexpr LittleEndian(T value) { *this = value; }

    constexpr LittleEndian operator=(T value) {
        m_value = host_to_little_endian(value);
        return *this;
    }

    constexpr operator T() const { return little_endian_to_host(m_value); }

private:
    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<LittleEndian>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<T, Enc>(parse_context, debug);
    }

    T m_value { 0 };
};
}