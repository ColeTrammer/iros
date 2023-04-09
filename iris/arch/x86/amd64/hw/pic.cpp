#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/core/global_state.h>
#include <iris/hw/irq.h>

namespace iris::x86::amd64 {
// See OSDEV for details on the x86 Programmable Interrupt Controller.
// https://wiki.osdev.org/8259_PIC

constexpr static u16 primary_io_base = 0x20;
constexpr static u16 secondary_io_base = 0xA0;

constexpr static u16 command_offset = 0;
constexpr static u16 data_offset = 1;

constexpr static u8 command_eoi = 0x20;

class Pic {
public:
    void remap(u8 primary_offset, u8 secondary_offset) {
        auto a = io_in<u8>(primary_io_base + data_offset);
        auto b = io_in<u8>(secondary_io_base + data_offset);

        io_out(primary_io_base + command_offset, (u8) 0x11);
        io_wait();
        io_out(secondary_io_base + command_offset, (u8) 0x11);
        io_wait();
        io_out(primary_io_base + data_offset, primary_offset);
        io_wait();
        io_out(secondary_io_base + data_offset, secondary_offset);
        io_wait();
        io_out(primary_io_base + data_offset, (u8) 4);
        io_wait();
        io_out(secondary_io_base + data_offset, (u8) 2);
        io_wait();

        io_out(primary_io_base + data_offset, (u8) 1);
        io_wait();
        io_out(secondary_io_base + data_offset, (u8) 1);
        io_wait();

        io_out(secondary_io_base + data_offset, b);
        io_out(primary_io_base + data_offset, a);
    }

private:
    friend void tag_invoke(di::Tag<send_eoi>, Pic&, IrqLine irq_line) {
        if (irq_line.raw_value() >= 8) {
            io_out(secondary_io_base + command_offset, command_eoi);
        }
        io_out(primary_io_base + command_offset, command_eoi);
    }

    friend void tag_invoke(di::Tag<disable_irq_line>, Pic&, IrqLine irq_line) {
        if (irq_line.raw_value() < 8) {
            u8 value = io_in<u8>(primary_io_base + data_offset) | 1 << irq_line.raw_value();
            io_out(primary_io_base + data_offset, value);
        } else {
            u8 value = io_in<u8>(secondary_io_base + data_offset) | 1 << (irq_line.raw_value() - 8);
            io_out(secondary_io_base + data_offset, value);
        }
    }

    friend void tag_invoke(di::Tag<enable_irq_line>, Pic&, IrqLine irq_line) {
        if (irq_line.raw_value() < 8) {
            u8 value = io_in<u8>(primary_io_base + data_offset) & ~(1 << irq_line.raw_value());
            io_out(primary_io_base + data_offset, value);
        } else {
            u8 value = io_in<u8>(secondary_io_base + data_offset) & ~(1 << (irq_line.raw_value() - 8));
            io_out(secondary_io_base + data_offset, value);
        }
    }

    friend IrqLineRange tag_invoke(di::Tag<responsible_irq_line_range>, Pic&) {
        return IrqLineRange(IrqLine(0), IrqLine(16));
    }
};

void init_pic() {
    auto pic = Pic {};

    pic.remap(32, 32 + 8);

    di::for_each(di::iota(IrqLine(0), IrqLine(16)), di::bind_front(disable_irq_line, di::ref(pic)));

    // Setup the PIT to fire every 1 ms.
    auto divisor = 1193182 / 1000;
    io_out(0x43, (u8) 0b00110110);
    io_out(0x40, u8(divisor & 0xFF));
    io_out(0x40, u8(divisor >> 8));

    *global_state_in_boot().irq_controllers.emplace_back(di::move(pic));
}
}
