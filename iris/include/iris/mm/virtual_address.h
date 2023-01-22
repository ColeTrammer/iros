#pragma once

#include <di/prelude.h>

namespace iris::mm {
class VirtualAddress {
public:
    using uptr = unsigned long;
    using iptr = long;

    VirtualAddress() = default;
    constexpr explicit VirtualAddress(uptr address) : m_address(address) {}

    constexpr uptr raw_address() const { return m_address; }

    constexpr VirtualAddress& operator+=(iptr b) {
        m_address += b;
        return *this;
    }
    constexpr VirtualAddress& operator-=(iptr b) {
        m_address -= b;
        return *this;
    }

    constexpr VirtualAddress& operator++() {
        ++m_address;
        return *this;
    }
    constexpr VirtualAddress operator++(int) {
        auto copy = *this;
        ++m_address;
        return copy;
    }

    constexpr VirtualAddress& operator--() {
        --m_address;
        return *this;
    }
    constexpr VirtualAddress operator--(int) {
        auto copy = *this;
        --m_address;
        return copy;
    }

private:
    constexpr friend VirtualAddress operator+(VirtualAddress a, iptr b) { return VirtualAddress(a.raw_address() + b); }
    constexpr friend VirtualAddress operator+(iptr a, VirtualAddress b) { return VirtualAddress(a + b.raw_address()); }
    constexpr friend VirtualAddress operator-(VirtualAddress a, iptr b) { return VirtualAddress(a.raw_address() - b); }
    constexpr friend VirtualAddress operator-(iptr a, VirtualAddress b) { return VirtualAddress(a - b.raw_address()); }
    constexpr friend iptr operator-(VirtualAddress a, VirtualAddress b) { return a.raw_address() - b.raw_address(); }
    constexpr friend di::strong_ordering operator<=>(VirtualAddress, VirtualAddress) = default;

    uptr m_address { 0 };
};
}