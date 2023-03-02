#pragma once

#include <di/types/prelude.h>

namespace iris {
namespace arch {
    constexpr inline di::u64 interrupt_enable_flag = 1 << 9;
}

static inline void raw_enable_interrupts() {
    asm volatile("sti");
}

static inline void raw_disable_interrupts() {
    asm volatile("cli");
}

static inline bool raw_disable_interrupts_and_save_previous_state() {
    di::u64 rflags;
    asm volatile("pushfq\n"
                 "pop %0\n"
                 : "=r"(rflags)
                 :
                 : "cc");
    if (!(rflags & arch::interrupt_enable_flag)) {
        return true;
    } else {
        raw_disable_interrupts();
        return false;
    }
}

class InterruptDisabler {
public:
    InterruptDisabler() { m_had_interrupts_disabled = raw_disable_interrupts_and_save_previous_state(); }

    ~InterruptDisabler() {
        if (!m_had_interrupts_disabled) {
            raw_enable_interrupts();
        }
    }

private:
    bool m_had_interrupts_disabled { false };
};
}
