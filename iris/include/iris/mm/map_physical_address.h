#pragma once

#include <di/meta/trivial.h>
#include <di/vocab/span/prelude.h>
#include <iris/core/error.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
struct PhysicalAddressMapping;

Expected<PhysicalAddressMapping> map_physical_address(PhysicalAddress, usize byte_size);

struct PhysicalAddressMapping {
public:
    explicit PhysicalAddressMapping(di::Span<di::Byte> data) : m_data(data) {}

    ~PhysicalAddressMapping() = default;

    PhysicalAddressMapping(PhysicalAddressMapping const&) = delete;
    PhysicalAddressMapping(PhysicalAddressMapping&&) = default;
    PhysicalAddressMapping& operator=(PhysicalAddressMapping const&) = delete;
    PhysicalAddressMapping& operator=(PhysicalAddressMapping&&) = default;

    template<typename T>
    T& typed() const {
        return *reinterpret_cast<T*>(m_data.data());
    }

private:
    di::Span<di::Byte> m_data;
};
}
