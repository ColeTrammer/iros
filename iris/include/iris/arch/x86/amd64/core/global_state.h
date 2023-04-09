#pragma once

#include <iris/hw/irq_controller.h>

namespace iris::arch {
struct ReadonlyGlobalState {
    GlobalIrqNumber external_irq_offset { 32 };
    bool use_apic { false };
};

struct MutableGlobalState {};
}
