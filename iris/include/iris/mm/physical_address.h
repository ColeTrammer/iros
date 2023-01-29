#pragma once

#include <di/prelude.h>

namespace iris::mm {
class PhysicalAddress {
public:
    constexpr explicit PhysicalAddress(u64 address) : m_address(address) {}

    constexpr u64 raw_address() const { return m_address; }

    constexpr PhysicalAddress& operator+=(u64 b) {
        m_address += b;
        return *this;
    }
    constexpr PhysicalAddress& operator-=(u64 b) {
        m_address -= b;
        return *this;
    }

private:
    constexpr friend PhysicalAddress operator+(PhysicalAddress a, u64 b) {
        return PhysicalAddress(a.raw_address() + b);
    }
    constexpr friend PhysicalAddress operator+(u64 a, PhysicalAddress b) {
        return PhysicalAddress(a + b.raw_address());
    }
    constexpr friend PhysicalAddress operator-(PhysicalAddress a, u64 b) {
        return PhysicalAddress(a.raw_address() - b);
    }
    constexpr friend i64 operator-(PhysicalAddress a, PhysicalAddress b) {
        return a.raw_address() - b.raw_address();
    }
    constexpr friend di::strong_ordering operator<=>(PhysicalAddress, PhysicalAddress) = default;

    u64 m_address { 0 };
};
}