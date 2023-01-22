#pragma once

#include <di/prelude.h>

namespace iris::mm {
class VirtualAddress {
public:
    using uintptr_t = unsigned long;
    using intptr_t = long;

    VirtualAddress() = default;
    constexpr explicit VirtualAddress(uintptr_t address) : m_address(address) {}

    constexpr uintptr_t raw_address() const { return m_address; }

    constexpr VirtualAddress& operator+=(intptr_t b) {
        m_address += b;
        return *this;
    }
    constexpr VirtualAddress& operator-=(intptr_t b) {
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
    constexpr friend VirtualAddress operator+(VirtualAddress a, intptr_t b) { return VirtualAddress(a.raw_address() + b); }
    constexpr friend VirtualAddress operator+(intptr_t a, VirtualAddress b) { return VirtualAddress(a + b.raw_address()); }
    constexpr friend VirtualAddress operator-(VirtualAddress a, intptr_t b) { return VirtualAddress(a.raw_address() - b); }
    constexpr friend VirtualAddress operator-(intptr_t a, VirtualAddress b) { return VirtualAddress(a - b.raw_address()); }
    constexpr friend intptr_t operator-(VirtualAddress a, VirtualAddress b) { return a.raw_address() - b.raw_address(); }
    constexpr friend di::strong_ordering operator<=>(VirtualAddress, VirtualAddress) = default;

    uintptr_t m_address { 0 };
};
}