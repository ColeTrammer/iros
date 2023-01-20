#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64::sd {
using namespace di::bit;

// The format of a 64 bit Segment Descriptor is described
// in the AMD64 Programmer's Manual; Volume 2; Figure 4-20 and 4-21,
// and on the OSDEV wiki at the page:
// https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor
//
// Note that the format for segment descriptors is identical between
// 32 bit and 64 bit x86, but in 64 bit mode, most fields are ignored.

struct LimitLow : BitField<0, 16> {};
struct BaseLow : BitField<16, 24> {};

// This is ignored on x86_64.
struct Accessed : BitFlag<40> {};
// This is ignored on x86_64, and should be set to 1. This is valid for code segments.
struct Readable : BitFlag<41> {};
// This is ignored on x86_64, and should be set to 1. This is valid for data segments.
struct Writable : BitFlag<41> {};

// This bit allows a control flow transfer to a segment of higher privledge without changing the current privledge level.
// This is valid for Code segments.
struct Conforming : BitFlag<42> {};
// This is ignored on x86_64, and should be set to 0. This is valid for Data segments.
struct ExpandDown : BitFlag<42> {};

// The segment loaded into the %cs register must have this bit set.
struct Code : BitFlag<43> {};

struct MustBeOne : BitFlag<44> {};

struct DPL : BitField<45, 2> {};
struct Present : BitFlag<47> {};

struct LimitHigh : BitField<48, 4> {};

struct Available : BitFlag<52> {};
// This is valid for Code segments.
struct LongMode : BitFlag<53> {};
struct Not16Bit : BitFlag<54> {};
struct Granular : BitFlag<55> {};

struct BaseHigh : BitField<56, 8> {};

using SegmentDescriptor = BitStruct<8, LimitLow, BaseLow, Accessed, Readable, Writable, Conforming, ExpandDown, Code, MustBeOne, DPL,
                                    Present, LimitHigh, Available, LongMode, Not16Bit, Granular>;
}