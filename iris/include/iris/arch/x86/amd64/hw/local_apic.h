#pragma once

#include <di/prelude.h>
#include <iris/mm/physical_address.h>

namespace iris::x86::amd64 {
/// @brief Local APIC Register Offsets
///
/// See [OSDEV](https://wiki.osdev.org/APIC#Local_APIC_registers) or AMD64 Programmer's Manual; Volume 2; Section 16.3.2
/// Figure 16-2.
enum class ApicOffset : u16 {
    Id = 0x0020,
    Version = 0x0030,
    TaskPriority = 0x0080,
    ArbitrationPriority = 0x0090,
    ProcessorPriority = 0x00A0,
    EndOfInterrupt = 0x00B0,
    RemoteRead = 0x00C0,
    LogicalDestination = 0x00D0,
    DestinationFormat = 0x00E0,
    SpuriousInterruptVector = 0x00F0,
    InService = 0x0100,
    TriggerMode = 0x0180,
    InterruptRequest = 0x0200,
    ErrorStatus = 0x0280,
    LvtTimer = 0x0320,
    InterruptCommandLow = 0x0300,
    InterruptCommandHigh = 0x0310,
    TimerLvtEntry,
    ThermalLvtEntry = 0x0330,
    PerformanceLvtEntry = 0x0340,
    Lint0Entry = 0x0350,
    Lint1Entry = 0x0360,
    ErrorEntry = 0x0370,
    TimerInitialCount = 0x0380,
    TimerCurrentCount = 0x0390,
    TimerDivideConfiguration = 0x03E0,
    ExtendedFeature = 0x0400,
    ExtendedControl = 0x0410,
    SpecificEndOfInterrupt = 0x0420,
    InterruptEnable = 0x0480,
    ExtendedLvtEntries = 0x0500,
};

struct ApicVersion : di::BitField<0, 8> {};
struct ApicMaxLvtEntry : di::BitField<16, 8> {};
struct ApicExtendedRegisterPresent : di::BitFlag<31> {};

/// @brief Local APIC Version Register
///
/// See AMD64 Programmer's Manual; Volume 2; Section 16.3.4 Figure 16-4.
using ApicVersionRegister = di::BitStruct<4, ApicVersion, ApicMaxLvtEntry, ApicExtendedRegisterPresent>;

struct ApicExtendedFeatureInterruptEnable : di::BitFlag<0> {};
struct ApicExtendedFeatureSpecificEoi : di::BitFlag<1> {};
struct ApicExtendedFeatureExtendedId : di::BitFlag<2> {};
struct ApicExtendedFeatureExtendedLvtCount : di::BitField<16, 8> {};

/// @brief Local APIC Extended Feature Register
///
/// See AMD64 Programmer's Manual; Volume 2; Section 16.3.5 Figure 16-5.
using ApicExtendedFeatureRegister = di::BitStruct<4, ApicExtendedFeatureInterruptEnable, ApicExtendedFeatureSpecificEoi,
                                                  ApicExtendedFeatureExtendedId, ApicExtendedFeatureExtendedLvtCount>;

struct ApicExtendedControlInterruptEnable : di::BitFlag<0> {};
struct ApicExtendedControlSpecificEoi : di::BitFlag<1> {};
struct ApicExtendedControlExtendId : di::BitFlag<2> {};

/// @brief Local APIC Extended Control Register
///
/// See AMD64 Programmer's Manual; Volume 2; Section 16.3.6 Figure 16-6.
using ApicExtendedControlRegister =
    di::BitStruct<4, ApicExtendedControlInterruptEnable, ApicExtendedControlSpecificEoi, ApicExtendedControlExtendId>;

/// @brief Local APIC Message Type
///
/// @warning Only Fixed, Smi, Nmi, and External are valid for LVT entries. All types are valid when sending IPIs.
///
/// See AMD64 Programmer's Manual; Volume 2; Section 16.5.
enum class ApicMessageType : u8 {
    Fixed = 0b000,
    LowestPriority = 0b001,
    Smi = 0b010,
    RemoteRead = 0b011,
    Nmi = 0b100,
    Init = 0b101,
    Startup = 0b110,
    External = 0b111,
};

struct ApicLvtEntryVector : di::BitField<0, 8> {};
struct ApicLvtEntryMessageType : di::BitEnum<ApicMessageType, 8, 3> {};
struct ApicLvtEntryDeliveryStatus : di::BitFlag<12> {};
struct ApicLvtEntryRemoteIrr : di::BitFlag<14> {};
struct ApicLvtEntryTriggerMode : di::BitFlag<15> {};
struct ApicLvtEntryMask : di::BitFlag<16> {};
struct ApicLvtEntryTimerMode : di::BitFlag<17> {};

/// @brief Local APIC LVT Entry
///
/// See [OSDEV](Local_Vector_Table_Registers) and AMD64 Programmer's Manual; Volume 2; Section 16.4 Figure 16-7.
using ApicLvtEntry =
    di::BitStruct<4, ApicLvtEntryVector, ApicLvtEntryMessageType, ApicLvtEntryDeliveryStatus, ApicLvtEntryRemoteIrr,
                  ApicLvtEntryTriggerMode, ApicLvtEntryMask, ApicLvtEntryTimerMode>;

/// @brief Local APIC Timer Divide Configuration
///
/// See AMD64 Programmer's Manual; Volume 2; Section 16.4.1 Figure 16-11.
enum class ApicTimerDivideConfiguration : u32 {
    DivideBy2 = 0b0000,
    DivideBy4 = 0b0001,
    DivideBy8 = 0b0010,
    DivideBy16 = 0b0011,
    DivideBy32 = 0b1000,
    DivideBy64 = 0b1001,
    DivideBy128 = 0b1010,
    DivideBy1 = 0b1011,
};

class LocalApic {
public:
    explicit LocalApic(mm::PhysicalAddress base);

    u32 direct_read(ApicOffset offset) const { return m_base[di::to_underlying(offset) / sizeof(u32)]; }
    void direct_write(ApicOffset offset, u32 value) { m_base[di::to_underlying(offset) / sizeof(u32)] = value; }

    /// @brief Read the Local APIC ID
    ///
    /// See AMD64 Programmer's Manual; Volume 2; Section 16.3.3 Figure 16-3.
    u16 id() const { return direct_read(ApicOffset::Id) >> 24; }

    ApicVersionRegister version() const { return di::bit_cast<ApicVersionRegister>(direct_read(ApicOffset::Version)); }

    ApicExtendedFeatureRegister extended_feature() const {
        return di::bit_cast<ApicExtendedFeatureRegister>(direct_read(ApicOffset::ExtendedFeature));
    }

    ApicExtendedControlRegister extended_control() const {
        return di::bit_cast<ApicExtendedControlRegister>(direct_read(ApicOffset::ExtendedControl));
    }
    void write_extended_control(ApicExtendedControlRegister value) {
        direct_write(ApicOffset::ExtendedControl, di::bit_cast<u32>(value));
    }

    void write_spurious_interrupt_vector(u8 value) { direct_write(ApicOffset::SpuriousInterruptVector, value); }

    ApicLvtEntry lvt_entry(ApicOffset offset) const { return di::bit_cast<ApicLvtEntry>(direct_read(offset)); }
    void write_lvt_entry(ApicOffset offset, ApicLvtEntry value) { direct_write(offset, di::bit_cast<u32>(value)); }

    u32 timer_initial_count() const { return direct_read(ApicOffset::TimerInitialCount); }
    void write_timer_initial_count(u32 value) { direct_write(ApicOffset::TimerInitialCount, value); }

    u32 timer_current_count() const { return direct_read(ApicOffset::TimerCurrentCount); }
    void write_timer_current_count(u32 value) { direct_write(ApicOffset::TimerCurrentCount, value); }

    ApicTimerDivideConfiguration timer_divide_configuration() const {
        return ApicTimerDivideConfiguration(direct_read(ApicOffset::TimerDivideConfiguration));
    }
    void write_timer_divide_configuration(ApicTimerDivideConfiguration value) {
        direct_write(ApicOffset::TimerDivideConfiguration, di::to_underlying(value));
    }

private:
    u32 volatile* m_base { nullptr };
};

void init_local_apic();
}
