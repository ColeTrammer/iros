#pragma once

#include <di/prelude.h>

namespace dius {
class MemoryRegion {
public:
    MemoryRegion() = default;

    constexpr explicit MemoryRegion(di::Span<di::Byte> data) : m_data(data) {}

    constexpr MemoryRegion(MemoryRegion&& other) : m_data(di::exchange(other.m_data, {})) {}

    ~MemoryRegion();

    constexpr MemoryRegion& operator=(MemoryRegion&& other) {
        m_data = di::exchange(other.m_data, {});
        return *this;
    }

    constexpr auto data() { return m_data.data(); }
    constexpr auto data() const { return m_data.data(); }

    constexpr auto size() const { return m_data.size(); }

    constexpr auto span() { return m_data; }
    constexpr auto span() const { return m_data; }

    constexpr bool empty() const { return m_data.empty(); }

private:
    di::Span<di::Byte> m_data;
};
}