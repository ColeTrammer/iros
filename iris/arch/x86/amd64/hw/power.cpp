#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/print.h>
#include <iris/hw/power.h>

namespace iris {
void hard_shutdown(ShutdownStatus status) {
    // NOTE: this is a fake isa-debug-exit device installed in the runner script.
    auto qemu_exit_code = status == ShutdownStatus::Intended ? 0x10_u32 : 0x11_u32;
    x86::amd64::io_out(0xf4_u16, qemu_exit_code);

    // NOTE: this is specific to QEMU, as per OSDEV:
    //       https://wiki.osdev.org/Shutdown
    x86::amd64::io_out(0x604_u16, 0x2000_u32);

    // If this fails, cause a triple fault.
    // This is pretty dramatic, it would be better
    // to mess with the IDT and explicitly stop handling
    // interrupts.
    x86::amd64::load_cr3(0);

    di::unreachable();
}
}
