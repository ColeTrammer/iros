#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64 {
// The format of the 64 bit Interrupt Descriptor Table is described
// in the AMD64 Programmer's Manual; Volume 2; Figure 12-8, and on
// the OSDEV wiki at the page:
// https://wiki.osdev.org/Task_State_Segment#Long_Mode
struct [[gnu::packed]] TSS {
    u32 reserved1;
    u64 rsp[3];
    u64 reserved2;
    u64 ist[7];
    u64 reserved3;
    u16 reserved4;
    u16 io_map_base;
};
}
