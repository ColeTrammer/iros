#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64::ssd {
using namespace di::bit;

// The format of the 64 bit System Segment Descriptor is described
// in the AMD64 Programmer's Manual; Volume 2; Figure 4-22, and on
// the OSDEV wiki at the page:
// https://wiki.osdev.org/Global_Descriptor_Table#Long_Mode_System_Segment_Descriptor
struct LimitLow : BitField<0, 16> {};
struct BaseLow : BitField<16, 16> {};
struct BaseMidLow : BitField<32, 8> {};

// The only relevant system segment descriptor for the GDT is the TSS.
struct Type : BitField<40, 4> {
    constexpr static u8 TSS = 0b1001;
};

struct DPL : BitField<45, 2> {};
struct Present : BitFlag<47> {};
struct LimitHigh : BitField<48, 16> {};
struct Granular : BitFlag<55> {};
struct BaseMidHigh : BitField<56, 8> {};
struct BaseHigh : BitField<64, 32> {};

using SystemSegmentDescriptor =
    BitStruct<16, LimitLow, BaseLow, BaseMidLow, Type, DPL, Present, LimitHigh, BaseMidHigh, Granular, BaseHigh>;

void init_tss();
}
