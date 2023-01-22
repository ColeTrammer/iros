#pragma once

#include <di/prelude.h>

namespace iris::mm {
class PhysicalAddress {
public:
    constexpr explicit PhysicalAddress(uptr address) : m_address(address) {}

    constexpr uptr raw_address() const { return m_address; }

    constexpr PhysicalAddress& operator+=(uptr b) {
        m_address += b;
        return *this;
    }
    constexpr PhysicalAddress& operator-=(uptr b) {
        m_address -= b;
        return *this;
    }

private:
    constexpr friend PhysicalAddress operator+(PhysicalAddress a, uptr b) { return PhysicalAddress(a.raw_address() + b); }
    constexpr friend PhysicalAddress operator+(uptr a, PhysicalAddress b) { return PhysicalAddress(a + b.raw_address()); }
    constexpr friend PhysicalAddress operator-(PhysicalAddress a, uptr b) { return PhysicalAddress(a.raw_address() - b); }
    constexpr friend intptr_t operator-(PhysicalAddress a, PhysicalAddress b) { return a.raw_address() - b.raw_address(); }
    constexpr friend di::strong_ordering operator<=>(PhysicalAddress, PhysicalAddress) = default;

    uptr m_address { 0 };
};
}