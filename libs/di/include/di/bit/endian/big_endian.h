#pragma once

#include <di/bit/endian/endian.h>
#include <di/bit/operation/byteswap.h>
#include <di/format/prelude.h>

namespace di::bit {
namespace detail {
    struct HostToBigEndianFunction {
        template<concepts::IntegralOrEnum T>
        constexpr T operator()(T value) const {
            if constexpr (Endian::Native == Endian::Big) {
                return value;
            } else {
                return byteswap(value);
            }
        }
    };
}

constexpr inline auto host_to_big_endian = detail::HostToBigEndianFunction {};
constexpr inline auto big_endian_to_host = detail::HostToBigEndianFunction {};

template<concepts::IntegralOrEnum T>
class [[gnu::packed]] BigEndian {
public:
    BigEndian() = default;

    constexpr BigEndian(T value) { *this = value; }

    constexpr BigEndian operator=(T value) {
        m_value = host_to_big_endian(value);
        return *this;
    }

    constexpr operator T() const { return big_endian_to_host(m_value); }

    constexpr T value() const { return *this; }

private:
    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<BigEndian>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<T, Enc>(parse_context, debug);
    }

    T m_value { 0 };
};
}

namespace di {
using bit::BigEndian;

using bit::big_endian_to_host;
using bit::host_to_big_endian;
}
