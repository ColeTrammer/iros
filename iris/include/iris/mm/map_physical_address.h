#pragma once

#include <di/meta/trivial.h>
#include <di/vocab/span/prelude.h>
#include <iris/core/error.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
struct PhysicalAddressMapping;

auto map_physical_address(PhysicalAddress, usize byte_size) -> Expected<PhysicalAddressMapping>;

struct PhysicalAddressMapping {
public:
    explicit PhysicalAddressMapping(di::Span<di::Byte> data) : m_data(data) {}

    ~PhysicalAddressMapping() = default;

    PhysicalAddressMapping(PhysicalAddressMapping const&) = delete;
    PhysicalAddressMapping(PhysicalAddressMapping&&) = default;
    auto operator=(PhysicalAddressMapping const&) -> PhysicalAddressMapping& = delete;
    auto operator=(PhysicalAddressMapping&&) -> PhysicalAddressMapping& = default;

    template<typename T>
    auto typed() const -> T& {
        return *reinterpret_cast<T*>(m_data.data());
    }

    auto span() const { return m_data; }

private:
    di::Span<di::Byte> m_data;
};
}
