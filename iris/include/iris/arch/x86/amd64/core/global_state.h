#pragma once

#include <iris/arch/x86/amd64/idt.h>
#include <iris/hw/irq_controller.h>

namespace iris::arch {
struct ReadonlyGlobalState {
    GlobalIrqNumber external_irq_offset { 32 };
    bool use_apic { false };
    di::Array<iris::x86::amd64::idt::Entry, 256> idt {};
};

struct MutableGlobalState {};
}
