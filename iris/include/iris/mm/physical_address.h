#pragma once

#include <di/prelude.h>

namespace iris::mm {
class PhysicalAddress {
public:
    using uintptr_t = unsigned long;
    using intptr_t = long;

    constexpr explicit PhysicalAddress(uintptr_t address) : m_address(address) {}

    constexpr uintptr_t raw_address() const { return m_address; }

    constexpr PhysicalAddress& operator+=(uintptr_t b) {
        m_address += b;
        return *this;
    }
    constexpr PhysicalAddress& operator-=(uintptr_t b) {
        m_address -= b;
        return *this;
    }

private:
    constexpr friend PhysicalAddress operator+(PhysicalAddress a, uintptr_t b) { return PhysicalAddress(a.raw_address() + b); }
    constexpr friend PhysicalAddress operator+(uintptr_t a, PhysicalAddress b) { return PhysicalAddress(a + b.raw_address()); }
    constexpr friend PhysicalAddress operator-(PhysicalAddress a, uintptr_t b) { return PhysicalAddress(a.raw_address() - b); }
    constexpr friend intptr_t operator-(PhysicalAddress a, PhysicalAddress b) { return a.raw_address() - b.raw_address(); }
    constexpr friend di::strong_ordering operator<=>(PhysicalAddress, PhysicalAddress) = default;

    uintptr_t m_address { 0 };
};
}