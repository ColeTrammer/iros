#pragma once

#include <liim/container/array.h>
#include <stdint.h>

namespace LIIM::Container::Hash {
class Hasher {
public:
    constexpr Hasher() {}

    constexpr void add(Span<const uint8_t> data) {
        for (auto byte : data) {
            m_running_hash <<= 8;
            m_running_hash |= byte;
        }
    }

    constexpr void add(Span<const char> data) {
        for (auto byte : data) {
            add(byte);
        }
    }

    template<Integral T>
    constexpr void add(T value) {
        auto bytes = bit_cast<Array<uint8_t, sizeof(value)>>(value);
        return add(bytes.span());
    }

    template<typename T>
    constexpr void add(T* pointer) {
        return add(static_cast<uintptr_t>(pointer));
    }

    constexpr uint64_t finish() { return m_running_hash; }

private:
    uint64_t m_running_hash { 0 };
};
}
