#include <iris/arch/x86/amd64/hw/pit.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/hw/irq.h>
#include <iris/hw/irq_controller.h>
#include <iris/hw/timer.h>

namespace iris::x86::amd64 {
constexpr static auto pit_frequency = 1193182;

class PitTimer {
private:
    Expected<void> register_irq() {
        if (m_irq_id) {
            return {};
        }

        m_irq_id = TRY(register_external_irq_handler(IrqLine(0), [this](IrqContext& context) -> IrqStatus {
            send_eoi(*context.controller->lock(), IrqLine(0));
            m_callback(context);
            return IrqStatus::Handled;
        }));
        return {};
    }

    friend di::StringView tag_invoke(di::Tag<timer_name>, PitTimer const&) { return "PIT"_sv; }

    friend TimerCapabilities tag_invoke(di::Tag<timer_capabilities>, PitTimer const&) {
        return TimerCapabilities::SingleShot | TimerCapabilities::Periodic;
    }

    friend TimerResolution tag_invoke(di::Tag<timer_resolution>, PitTimer const&) {
        return di::Nanoseconds(1000000000 / pit_frequency);
    }

    friend Expected<void> tag_invoke(di::Tag<timer_set_single_shot>, PitTimer& self, di::Nanoseconds duration,
                                     di::Function<void(IrqContext&)> callback) {
        auto divisor = duration.count() * pit_frequency / 1000000000;

        return with_interrupts_disabled([&] -> Expected<void> {
            TRY(self.register_irq());
            self.m_callback = std::move(callback);
            // Set PIT to mode 0: interrupt on terminal count.
            x86::amd64::io_out(0x43, 0b00110000_u8);
            x86::amd64::io_out(0x40, u8(divisor & 0xFF));
            x86::amd64::io_out(0x40, u8(divisor >> 8));
            return {};
        });
    }

    friend Expected<void> tag_invoke(di::Tag<timer_set_interval>, PitTimer& self, di::Nanoseconds duration,
                                     di::Function<void(IrqContext&)> callback) {
        auto divisor = duration.count() * pit_frequency / 1000000000;

        return with_interrupts_disabled([&] -> Expected<void> {
            TRY(self.register_irq());
            self.m_callback = std::move(callback);

            // Set PIT to mode 3: square wave generator.
            x86::amd64::io_out(0x43, 0b00110110_u8);
            x86::amd64::io_out(0x40, u8(divisor & 0xFF));
            x86::amd64::io_out(0x40, u8(divisor >> 8));
            return {};
        });
    }

    di::Function<void(IrqContext&)> m_callback;
    di::Optional<usize> m_irq_id;
};

static_assert(di::Impl<PitTimer, TimerInterface>);

void init_pit() {
    // FIXME: In the future, we should determine whether the HPET is available, and if so, disable the PIT.

    *global_state_in_boot().timers.emplace_back(*Timer::try_create(di::in_place_type<PitTimer>));
}
}
