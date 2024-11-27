#pragma once

#include <di/types/prelude.h>

namespace di::bit::detail {
class BitProxyReference {
public:
    constexpr explicit BitProxyReference(u8* byte, u8 bit_offset) : m_byte(byte), m_bit_offset(bit_offset) {}

    constexpr BitProxyReference(BitProxyReference const&) = default;
    constexpr BitProxyReference(BitProxyReference&&) = default;

    constexpr auto operator=(bool value) const -> BitProxyReference const& {
        if (value) {
            *m_byte |= (1u << m_bit_offset);
        } else {
            *m_byte &= ~(1u << m_bit_offset);
        }
        return *this;
    }
    constexpr auto operator=(BitProxyReference const& other) const -> BitProxyReference const& {
        return *this = bool(other);
    }

    constexpr operator bool() const { return !!(*m_byte & (1u << m_bit_offset)); }

    constexpr auto operator~() const -> bool { return !bool(*this); }

    constexpr auto flip() const -> BitProxyReference const& {
        *m_byte ^= (1u << m_bit_offset);
        return *this;
    }

private:
    u8* m_byte;
    u8 m_bit_offset;
};
}
