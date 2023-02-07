#include <iris/arch/x86/amd64/io_instructions.h>

namespace iris::x86::amd64 {
// See OSDEV for details on the x86 Programmable Interrupt Controller.
// https://wiki.osdev.org/8259_PIC

constexpr static u16 primary_io_base = 0x20;
constexpr static u16 secondary_io_base = 0xA0;

constexpr static u16 command_offset = 0;
constexpr static u16 data_offset = 1;

constexpr static u8 command_eoi = 0x20;

void send_eoi(u8 irq_line) {
    if (irq_line >= 8) {
        io_out(secondary_io_base + command_offset, command_eoi);
    }
    io_out(primary_io_base + command_offset, command_eoi);
}

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

void disable_irq_line(u8 irq_line) {
    if (irq_line < 8) {
        u8 value = io_in<u8>(primary_io_base + data_offset) | 1 << irq_line;
        io_out(primary_io_base + data_offset, value);
    } else {
        u8 value = io_in<u8>(secondary_io_base + data_offset) | 1 << (irq_line - 8);
        io_out(secondary_io_base + data_offset, value);
    }
}

void enable_irq_line(u8 irq_line) {
    if (irq_line < 8) {
        u8 value = io_in<u8>(primary_io_base + data_offset) & ~(1 << irq_line);
        io_out(primary_io_base + data_offset, value);
    } else {
        u8 value = io_in<u8>(secondary_io_base + data_offset) & ~(1 << (irq_line - 8));
        io_out(secondary_io_base + data_offset, value);
    }
}

void init_pic() {
    remap(32, 40);

    di::for_each(di::range(16), disable_irq_line);
}
}