#pragma once

#include <di/assert/prelude.h>
#include <di/bit/prelude.h>
#include <di/function/prelude.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/hw/acpi/acpi.h>
#include <iris/hw/irq_controller.h>
#include <iris/mm/physical_address.h>

/// @file
/// @brief IO APIC Definitions
///
/// See [OSDEV](https://wiki.osdev.org/IOAPIC) or Intel's 82093AA IOAPIC Specification.
/// A PDF of the specification can be found [here](https://pdos.csail.mit.edu/6.828/2017/readings/ia32/ioapic.pdf).

namespace iris::x86::amd64 {
/// @brief IO APIC Access Register Offsets
///
/// See Intel's 82093AA IOAPIC Specification; Section 3.0. Table 1.
enum class IoApicAccessOffset : u8 {
    RegisterSelect = 0x00,
    Window = 0x10,
};

/// @brief IO APIC Register Offsets
///
/// See Intel's 82093AA IOAPIC Specification; Section 3.0. Table 2.
enum class IoApicOffset : u8 {
    Id = 0x00,
    Version = 0x01,
    ArbitrationId = 0x02,
    RedirectionTable = 0x10,
};

/// @brief IO APIC Version Register
///
/// See Intel's 82093AA IOAPIC Specification; Section 3.2.2.
struct IoApicVersion : di::BitField<0, 8> {};
struct IoApicMaxRedirectionEntry : di::BitField<16, 8> {};
using IoApicVersionRegister = di::BitStruct<4, IoApicVersion, IoApicMaxRedirectionEntry>;

/// @brief IO APIC Destination Mode
///
/// See Intel's 82093AA IOAPIC Specification; Section 3.2.4.
enum class IoApicDestinationMode {
    Physical = 0, ///< Send to a single processor.
    Logical = 1,  ///< Send to a bit mask of processors.
};

/// @brief IO APIC Redirection Table Entry
///
/// See Intel's 82093AA IOAPIC Specification; Section 3.2.4.
struct IoApicRedirectionTableEntryVector : di::BitField<0, 8> {};
struct IoApicRedirectionTableEntryDeliveryMode : di::BitEnum<ApicMessageType, 8, 3> {};
struct IoApicRedirectionTableEntryDestinationMode : di::BitEnum<IoApicDestinationMode, 11, 1> {};
struct IoApicRedirectionTableEntryDeliveryStatus : di::BitFlag<12> {};
struct IoApicRedirectionTableEntryPolarity : di::BitFlag<13> {};
struct IoApicRedirectionTableEntryRemoteIrr : di::BitFlag<14> {};
struct IoApicRedirectionTableEntryTriggerMode : di::BitFlag<15> {};
struct IoApicRedirectionTableEntryMask : di::BitFlag<16> {};
struct IoApicRedirectionTableEntryDestination : di::BitField<56, 8> {};
using IoApicRedirectionTableEntry =
    di::BitStruct<8, IoApicRedirectionTableEntryVector, IoApicRedirectionTableEntryDeliveryMode,
                  IoApicRedirectionTableEntryDestinationMode, IoApicRedirectionTableEntryDeliveryStatus,
                  IoApicRedirectionTableEntryPolarity, IoApicRedirectionTableEntryRemoteIrr,
                  IoApicRedirectionTableEntryTriggerMode, IoApicRedirectionTableEntryMask,
                  IoApicRedirectionTableEntryDestination>;

class IoApic {
public:
    IoApic(mm::PhysicalAddress access_base, u8 global_offset);

    u8 id() const { return m_id; }
    u8 max_redirection_entry() const { return m_max_redirection_entry; }

    u32 direct_read(IoApicOffset);
    u64 direct_read64(IoApicOffset offset) {
        auto low = direct_read(offset);
        auto high = direct_read(IoApicOffset(di::to_underlying(offset) + 1));
        return (static_cast<u64>(high) << 32) | low;
    }
    void direct_write(IoApicOffset, u32);
    void direct_write64(IoApicOffset offset, u64 value) {
        direct_write(offset, static_cast<u32>(value));
        direct_write(IoApicOffset(di::to_underlying(offset) + 1), static_cast<u32>(value >> 32));
    }

    IoApicRedirectionTableEntry read_redirection_entry(u8 offset) {
        return di::bit_cast<IoApicRedirectionTableEntry>(direct_read64(offset_for_redirection_entry(offset)));
    }
    void write_redirection_entry(u8 offset, IoApicRedirectionTableEntry entry) {
        direct_write64(offset_for_redirection_entry(offset), di::bit_cast<u64>(entry));
    }

private:
    friend void tag_invoke(di::Tag<send_eoi>, IoApic&, IrqLine irq_line);
    friend void tag_invoke(di::Tag<disable_irq_line>, IoApic&, IrqLine irq_line);
    friend void tag_invoke(di::Tag<enable_irq_line>, IoApic&, IrqLine irq_line);
    friend IrqLineRange tag_invoke(di::Tag<responsible_irq_line_range>, IoApic const&);

    IoApicOffset offset_for_redirection_entry(u8 number) {
        ASSERT_LT_EQ(number, m_max_redirection_entry);
        return IoApicOffset(di::to_underlying(IoApicOffset::RedirectionTable) + 2 * number);
    }

    di::Tuple<u8, di::Optional<acpi::InterruptSourceOverrideStructure>> resolve_irq_line(IrqLine irq_line);

    u32 volatile* m_access { nullptr };
    u8 m_global_offset { 0 };
    u8 m_id { 0 };
    u8 m_max_redirection_entry { 0 };
};

void init_io_apic();
}
