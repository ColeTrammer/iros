#pragma once

#include <di/prelude.h>

namespace iris::acpi {
bool validate_acpi_checksum(di::Span<byte const> data);

/// @brief Root System Description Pointer
///
/// See https://wiki.osdev.org/RSDP or ACPI 6.5 spec section 5.2.5.3.
struct [[gnu::packed]] RSDP {
    /// @name Version 1.0 fields
    /// @{
    di::Array<byte, 8> signature;
    u8 checksum;
    di::Array<byte, 6> oem_id;
    u8 revision;
    u32 rsdt_address;
    /// @}

    /// @name Version 2.0+ fields
    /// @warning These fields are only valid if `revision >= 2`.
    /// @{
    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    di::Array<byte, 3> reserved;
    /// @}

    bool validate_v1() const { return validate_acpi_checksum(di::as_bytes(di::Span { this, 1 }) | di::take(20)); }
};

/// @brief System Description Table Header
///
/// See https://wiki.osdev.org/RSDT#structure or ACPI 6.5 spec section 5.2.6.
struct [[gnu::packed]] SDTHeader {
    di::Array<byte, 4> signature;
    u32 length;
    u8 revision;
    u8 checksum;
    di::Array<byte, 6> oem_id;
    di::Array<byte, 8> oem_table_id;
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;

    di::Span<byte const> as_bytes() const { return di::Span { reinterpret_cast<byte const*>(this), this->length }; }

    bool validate(usize min_length) const {
        if (this->length < min_length) {
            return false;
        }
        return validate_acpi_checksum(this->as_bytes());
    }
};

/// @brief Root System Description Table
///
/// See https://wiki.osdev.org/RSDT or ACPI 6.5 spec section 5.2.7.
struct [[gnu::packed]] RSDT : SDTHeader {
    /// @brief Array of pointers to other SDTs.
    di::Span<u32 const> entries() const {
        return di::Span { reinterpret_cast<u32 const*>(this + 1), (this->length - sizeof(SDTHeader)) / sizeof(u32) };
    }
};

template<usize N>
constexpr auto byte_array_to_string_view(di::Array<byte, N> const& array) {
    return di::TransparentStringView { reinterpret_cast<char const*>(array.data()), array.size() };
}
}
