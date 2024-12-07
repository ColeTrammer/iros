#include "iris/arch/x86/amd64/hw/sb16.h"

#include "di/container/algorithm/fill.h"
#include "di/util/compiler_barrier.h"
#include "di/vocab/expected/expected_forward_declaration.h"
#include "iris/arch/x86/amd64/core/interrupt_disabler.h"
#include "iris/arch/x86/amd64/io_instructions.h"
#include "iris/arch/x86/amd64/page_structure.h"
#include "iris/core/interrupt_disabler.h"
#include "iris/core/print.h"
#include "iris/core/userspace_access.h"
#include "iris/hw/irq.h"
#include "iris/hw/irq_controller.h"
#include "iris/mm/map_physical_address.h"
#include "iris/mm/page_frame_allocator.h"
#include "iris/mm/physical_address.h"

// References:
// - OSDEV:
//     https://wiki.osdev.org/Sound_Blaster_16
// - SerenityOS:
//     They seem to have deleted the driver at some point, but this old version is still in the git history.
//     https://github.com/SerenityOS/serenity/blob/f2faf11d614d1db373a2c9d0ad47540cd549239b/Kernel/Devices/SB16.cpp
//
// This driver is very hacky and only serves as a proof of concept. Implementing the AC97 card would be preferable but
// would require implementing PCI support.

namespace iris::x86::amd64 {
constexpr auto dsp_read_port = 0x22A_u16;
constexpr auto dsp_write_port = 0x22C_u16;
constexpr auto dsp_status_port = 0x22E_u16;
constexpr auto dsp_ack_port = 0x22F_u16;

constexpr auto dma_max_page_count = 8_u32;

static auto dma_base_addr = mm::PhysicalAddress(0);
static auto volatile got_irq = false;

static void write_dsp(byte value) {
    while ((x86::amd64::io_in<byte>(dsp_write_port) & 0x80_b) != 0_b) {}
    x86::amd64::io_out(dsp_write_port, value);
}

static void set_sample_rate(u16 hz) {
    write_dsp(0x41_b);
    write_dsp((byte) (hz >> 8));
    write_dsp((byte) hz);
}

static void start_dma(u32 length) {
    // Channel 5 for 16 bit.
    auto const channel = 5_u8;
    auto const mode = 0x48_b;

    x86::amd64::io_out(0xd4, byte(4 + (channel % 4)));
    x86::amd64::io_out(0xd8, 0_b);
    x86::amd64::io_out(0xd6, (byte) (channel % 4) | mode);

    auto addr = dma_base_addr.raw_value();
    u16 offset = (addr / 2) % 65536;
    x86::amd64::io_out(0xc4, (byte) offset);
    x86::amd64::io_out(0xc4, (byte) (offset >> 8));

    x86::amd64::io_out(0xc6, (byte) (length - 1));
    x86::amd64::io_out(0xc6, (byte) ((length - 1) >> 8));

    x86::amd64::io_out(0x8b, byte(addr >> 16));

    x86::amd64::io_out(0xd4, byte(channel % 4));
}

static void write_data(u32 length) {
    set_sample_rate(44100);

    start_dma(length);

    auto sample_count = length / sizeof(i16);
    sample_count /= 2; // Stereo is assumed
    sample_count -= 1;

    // SB 16 config
    write_dsp(0xB0_b);
    write_dsp(0x30_b); // Flags mean signed + stereo
    write_dsp(byte(sample_count));
    write_dsp(byte(sample_count >> 8));
}

static void irq_handler() {
    // Stop previous transfer
    write_dsp(0xD5_b);

    x86::amd64::io_in<byte>(dsp_status_port);
    x86::amd64::io_in<byte>(dsp_ack_port);

    got_irq = true;
}

static void finish_init() {
    dma_base_addr = *mm::allocate_physically_contiguous_page_frames(dma_max_page_count);

    println("Sound Blaster 16 DMA base address: {}"_sv, dma_base_addr);

    // ISA DMA requires memory allocated below 24 bits.
    ASSERT(dma_base_addr <= mm::PhysicalAddress(0x1'00'00'00));

    *register_external_irq_handler(IrqLine(5), [](IrqContext& context) {
        irq_handler();
        send_eoi(*context.controller->lock(), IrqLine(5));
        return IrqStatus::Handled;
    });

    // Enable speaker
    write_dsp(0xD1_b);
}

void init_sb16() {
    x86::amd64::io_out(0x226, 1_b);
    x86::amd64::io_wait_us(3);
    x86::amd64::io_out(0x226, 0_b);

    auto dsp_value = x86::amd64::io_in<byte>(dsp_read_port);
    if (dsp_value == 0xAA_b) {
        println("Found ISA Sound Blaster 16 Card"_sv);

        finish_init();
    } else {
        println("Did not find ISA Sound Blaster 16 Card"_sv);
    }
}

auto sb16_write_audio(UserspaceBuffer<byte const> data) -> Expected<usize> {
    if (data.size_bytes() > 4096_usize * dma_max_page_count || data.size() % 2 == 1) {
        return di::Unexpected(iris::Error::InvalidArgument);
    }

    auto memory = TRY(mm::map_physical_address(dma_base_addr, dma_max_page_count * 4096_usize));

    TRY(data.copy_to(memory.span()));

    with_interrupts_disabled([&] {
        write_data(u32(data.size()));
    });

    ASSERT(!iris::interrupts_disabled());

    // TODO: busy polling isn't it.
    while (!got_irq) {
        di::compiler_barrier();
        io_wait();
    }
    got_irq = false;

    return data.size();
}
}
