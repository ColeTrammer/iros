#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64::idt {
using namespace di::bit;

// The format of the 64 bit Interrupt Descriptor Table is described
// in the AMD64 Programmer's Manual; Volume 2; Figure 4-24, and on
// the OSDEV wiki at the page:
// https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
struct TargetLow : BitField<0, 16> {};
struct SegmentSelector : BitField<16, 16> {};
struct IST : BitField<32, 3> {};

// Interrupt Gates automatically disable interrupts upon entry, while Trap Gates do not.
// In practice, the system call vector should be made a Trap Gate, while hw IRQ handlers
// and CPU exception handlers should be made Interrupt Gates.
struct Type : BitField<40, 4> {
    constexpr static u8 InterruptGate = 0b1110;
    constexpr static u8 TrapGate = 0b1111;
};

struct DPL : BitField<45, 2> {};
struct Present : BitFlag<47> {};
struct TargetMid : BitField<48, 16> {};
struct TargetHigh : BitField<64, 32> {};

using Entry = BitStruct<16, TargetLow, SegmentSelector, IST, Type, DPL, Present, TargetMid, TargetHigh>;

void init_idt();
void load_idt();
}
