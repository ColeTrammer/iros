#include <iris/arch/x86/amd64/hw/serial.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/irq.h>

namespace iris::x86::amd64 {
void init_serial_early_boot() {
    // This code snippet initializes serial the ISA serial port. This is a hack to get debug output and easy user
    // interactivity. This is directly from https://wiki.osdev.org/Serial_Ports#Initialization.
    x86::amd64::io_out(0x3F8 + 1, 0x00_b); // Disable all interrupts
    x86::amd64::io_out(0x3F8 + 3, 0x80_b); // Enable DLAB (set baud rate divisor)
    x86::amd64::io_out(0x3F8 + 0, 0x03_b); // Set divisor to 3 (lo byte) 38400 baud
    x86::amd64::io_out(0x3F8 + 1, 0x00_b); //                  (hi byte)
    x86::amd64::io_out(0x3F8 + 3, 0x03_b); // 8 bits, no parity, one stop bit
    x86::amd64::io_out(0x3F8 + 2, 0xC7_b); // Enable FIFO, clear them, with 14-byte threshold
    x86::amd64::io_out(0x3F8 + 4, 0x0B_b); // IRQs enabled, RTS/DSR set
    x86::amd64::io_out(0x3F8 + 4, 0x1E_b); // Set in loopback mode, test the serial chip
    x86::amd64::io_out(0x3F8 + 0, 0xAE_b); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (x86::amd64::io_in<di::Byte>(0x3F8 + 0) != 0xAE_b) {
        iris::println("Failed to detect serial port."_sv);
    } else {
        // If serial is not faulty set it in normal operation mode
        // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
        x86::amd64::io_out(0x3F8 + 4, 0x0F_b);
        // Enable rx IRQs
        x86::amd64::io_out(0x3F8 + 1, 0x01_b);
        iris::println("Enabled serial port."_sv);
    }
}

void init_serial() {
    *register_external_irq_handler(IrqLine(4), [](IrqContext&) {
        while ((x86::amd64::io_in<u8>(0x3F8 + 5) & 1) == 0) {
            ;
        }

        auto byte = x86::amd64::io_in<di::Byte>(0x3F8);
        if (byte == '\r'_b) {
            byte = '\n'_b;
        }

        log_output_byte(byte);

        global_state().input_wait_queue.notify_one([&] {
            (void) global_state().input_data_queue.push(byte);
        });

        send_eoi(*global_state().irq_controller.lock(), IrqLine(4));
        return IrqStatus::Handled;
    });
}
}

namespace iris {
void log_output_byte(di::Byte byte) {
    while ((x86::amd64::io_in<di::Byte>(0x3F8 + 5) & 0x20_b) == 0_b)
        ;

    x86::amd64::io_out(0x3F8, byte);
}
}
