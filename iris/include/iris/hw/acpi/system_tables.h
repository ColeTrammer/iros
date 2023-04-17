#pragma once

#include <di/container/string/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/span/prelude.h>

namespace iris::acpi {
bool validate_acpi_checksum(di::Span<byte const> data);

/// @brief Root System Description Pointer
///
/// See [OSDEV](https://wiki.osdev.org/RSDP) or ACPI 6.5 spec section 5.2.5.
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
/// See [OSDEV](https://wiki.osdev.org/RSDT#structure) or ACPI 6.5 spec section 5.2.6.
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
/// See [OSDEV](https://wiki.osdev.org/RSDT) or ACPI 6.5 spec section 5.2.7.
struct [[gnu::packed]] RSDT : SDTHeader {
    di::Span<u32 const> entries() const {
        return di::Span { reinterpret_cast<u32 const*>(this + 1), (this->length - sizeof(SDTHeader)) / sizeof(u32) };
    }
};

/// @brief Interrupt Controller Structure Type
///
/// See ACPI 6.5 spec section 5.2.12 table 5-21.
enum class InterruptControllerStructureType : u8 {
    LocalApic = 0x0,
    IoApic = 0x1,
    InterruptSourceOverride = 0x2,
    NmiSource = 0x3,
    LocalApicNmi = 0x4,
    LocalApicAddressOverride = 0x5,
    IoSapic = 0x6,
    LocalSapic = 0x7,
    PlatformInterruptSources = 0x8,
    LocalX2Apic = 0x9,
    LocalX2ApicNmi = 0xA,
    GicCpuInterface = 0xB,
    GicDistrutor = 0xC,
    GicMsiFrame = 0xD,
    GicRedistributor = 0xE,
    GIGInterruptTranslationService = 0xF,
    MultiprocessorWakeup = 0x10,
    CorePic = 0x11,
    LegacyPic = 0x12,
    HyperTransportPic = 0x13,
    ExtendIoPic = 0x14,
    MsiPic = 0x15,
    BridgeIoPic = 0x16,
    LowPinCountPic = 0x17,
};

/// @brief Interrupt Controller Structure Header
///
/// See ACPI 6.5 spec section 5.2.12.
struct InterruptControllerStructureHeader {
    InterruptControllerStructureType type;
    u8 length;

    InterruptControllerStructureHeader const* next() const {
        return reinterpret_cast<InterruptControllerStructureHeader const*>(reinterpret_cast<byte const*>(this) +
                                                                           this->length);
    }

    bool validate(usize min_length) const { return this->length >= min_length; }
};

/// @brief Processor Local APIC Structure
///
/// See [OSDEV](https://wiki.osdev.org/MADT#Entry_Type_0:_Processor_Local_APIC) or ACPI 6.5 spec section 5.2.12.2.
struct [[gnu::packed]] ProcessorLocalApicStructure : InterruptControllerStructureHeader {
    enum class Flags : u32 {
        Enabled = 1,
        OnlineCapable = 2,
    };

    u8 processor_id;
    u8 apic_id;
    Flags flags;
};

/// @brief I/O APIC Structure
///
/// See [OSDEV](https://wiki.osdev.org/MADT#Entry_Type_1:_I.2FO_APIC) or ACPI 6.5 spec section 5.2.12.3.
struct [[gnu::packed]] IoApicStructure : InterruptControllerStructureHeader {
    u8 io_apic_id;
    u8 reserved;
    u32 io_apic_address;
    u32 global_system_interrupt_base;
};

/// @brief MPS Interrupt Flags
///
/// @note These flags apply to several structures, and are originally defined in the MPS 1.4 spec, but now referenced by
/// the ACPI 6.5 spec.
///
/// See [OSDEV](https://wiki.osdev.org/MADT#Flags) or ACPI 6.5 spec section 5.2.12.5 table 5-26.
struct [[gnu::packed]] MPSInterruptFlags {
    enum class Polarity {
        BusDefined = 0,
        ActiveHigh = 1,
        Reserved = 2,
        ActiveLow = 3,
    };

    enum class TriggerMode {
        BusDefined = 0,
        Edge = 1,
        Reserved = 2,
        Level = 3,
    };

    Polarity polarity() const { return Polarity(value & 0b11); }
    TriggerMode trigger_mode() const { return TriggerMode((value & 0b1100) >> 2); }

    u16 value;
};

/// @brief Interrupt Source Override Structure
///
/// See [OSDEV](https://wiki.osdev.org/MADT#Entry_Type_2:_IO.2FAPIC_Interrupt_Source_Override) or ACPI 6.5 spec
/// section 5.2.12.5.
struct [[gnu::packed]] InterruptSourceOverrideStructure : InterruptControllerStructureHeader {
    u8 bus;
    u8 source;
    u32 global_system_interrupt;
    MPSInterruptFlags flags;
};

/// @brief Local APIC NMI Structure
///
/// See [OSDEV](https://wiki.osdev.org/MADT#Entry_Type_4:_Local_APIC_Non-maskable_interrupts) or ACPI 6.5 spec
/// section 5.2.12.17.
struct [[gnu::packed]] LocalApicNmiStructure : InterruptControllerStructureHeader {
    u8 apic_processor_uid;
    MPSInterruptFlags flags;
    u8 local_apic_lint;
};

class InterruptControllerStructureIterator
    : public di::container::IteratorBase<InterruptControllerStructureIterator, di::ForwardIteratorTag,
                                         InterruptControllerStructureHeader, isize> {
public:
    InterruptControllerStructureIterator() = default;

    explicit InterruptControllerStructureIterator(InterruptControllerStructureHeader const* current,
                                                  InterruptControllerStructureHeader const* end)
        : m_current(current), m_end(end) {}

    InterruptControllerStructureHeader const& operator*() const { return *m_current; }
    InterruptControllerStructureHeader const* operator->() const { return m_current; }

    bool validate(usize min_size) const {
        auto const* readable_end = reinterpret_cast<byte const*>(m_current) + min_size;
        return m_current->validate(min_size) && m_current->next() <= m_end &&
               di::to_uintptr(readable_end) <= di::to_uintptr(m_end);
    }

    void advance_one() { m_current = m_current->next(); }

    InterruptControllerStructureIterator begin() const { return *this; }
    InterruptControllerStructureIterator end() const { return InterruptControllerStructureIterator { m_end, m_end }; }

private:
    friend bool operator==(InterruptControllerStructureIterator const& a,
                           InterruptControllerStructureIterator const& b) {
        return a.m_current == b.m_current;
    }

    InterruptControllerStructureHeader const* m_current { nullptr };
    InterruptControllerStructureHeader const* m_end { nullptr };
};

/// @brief Multiple APIC Description Table
///
/// See [OSDEV](https://wiki.osdev.org/MADT) or ACPI 6.5 spec section 5.2.12.
struct [[gnu::packed]] MADT : SDTHeader {
    enum class Flags : u32 {
        PcAtCompatible = 1,
    };

    u32 local_apic_address;
    Flags flags;

    InterruptControllerStructureIterator interrupt_controller_structures() const {
        return InterruptControllerStructureIterator {
            reinterpret_cast<InterruptControllerStructureHeader const*>(this + 1),
            reinterpret_cast<InterruptControllerStructureHeader const*>(this->as_bytes().end())
        };
    }
};

template<usize N>
constexpr auto byte_array_to_string_view(di::Array<byte, N> const& array) {
    return di::TransparentStringView { reinterpret_cast<char const*>(array.data()), array.size() };
}
}
