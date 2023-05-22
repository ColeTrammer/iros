#include <di/vocab/pointer/prelude.h>
#include <iris/arch/x86/amd64/core/interrupt_disabler.h>
#include <iris/arch/x86/amd64/core/processor.h>
#include <iris/arch/x86/amd64/gdt.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/irq.h>
#include <iris/hw/timer.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/map_physical_address.h>

namespace iris {
void Processor::handle_pending_ipi_messages() {
    // This takes the lock, and then empties the queue into a local variable.
    auto messages = m_ipi_message_queue.with_lock([](auto& queue) {
        auto result = di::move(queue);
        queue.clear();
        return result;
    });

    for (auto* message : messages) {
        if (message->tlb_flush_size > 0) {
            // NOTE: the caller is responsible for freeing the message.
            flush_tlb_local(message->tlb_flush_base, message->tlb_flush_size);
            message->times_processed.fetch_add(1, di::MemoryOrder::Release);
        }

        if (message->task_to_schedule != nullptr) {
            // NOTE: we are responsible for freeing the message.
            scheduler().schedule_task(*message->task_to_schedule);

            global_state().ipi_message_pool.lock()->deallocate(*message);
        }
    }
}

void Processor::send_ipi(u32 target_processor_id, di::FunctionRef<void(IpiMessage&)> factory) {
    ASSERT(di::math::representable_as<u8>(target_processor_id));
    auto processor_id = static_cast<u8>(target_processor_id);

    // Find the target processor.
    auto const& global_state = iris::global_state();
    auto* target_processor = *global_state.processor_map.at(processor_id);

    // Get IPI message from global object pool.
    auto& message = *global_state.ipi_message_pool.lock()->allocate();

    factory(message);

    // Add the message to the target's queue.
    *target_processor->m_ipi_message_queue.lock()->push(&message);

    // Send the IPI.
    auto& local_apic = arch_processor().local_apic();
    while (local_apic.interrupt_command_register().get<x86::amd64::ApicInterruptCommandDeliveryStatus>()) {
        x86::amd64::io_wait_us(20);
    }
    local_apic.write_interrupt_command_register(x86::amd64::ApicInterruptCommandRegister(
        x86::amd64::ApicInterruptCommandVector(63), x86::amd64::ApicInterruptCommandDestination(target_processor_id)));
}

void Processor::broadcast_ipi(di::FunctionRef<void(IpiMessage&)> factory) {
    // Get IPI message from global object pool.
    auto& message = *iris::global_state().ipi_message_pool.lock()->allocate();

    factory(message);

    // Add the message each processor's queue.
    auto count = 0_u32;
    for (auto [_, processor] : iris::global_state().processor_map) {
        if (processor == this) {
            continue;
        }
        *processor->m_ipi_message_queue.lock()->push(&message);
        count++;
    }

    // Send the IPI.
    auto& local_apic = arch_processor().local_apic();
    while (local_apic.interrupt_command_register().get<x86::amd64::ApicInterruptCommandDeliveryStatus>()) {
        x86::amd64::io_wait_us(20);
    }
    local_apic.write_interrupt_command_register(x86::amd64::ApicInterruptCommandRegister(
        x86::amd64::ApicInterruptCommandVector(63),
        x86::amd64::ApicInterruptCommandDestinationShorthand(x86::amd64::ApicDestinationShorthand::AllExcludingSelf)));

    // Wait for the message to be processed by all processors.
    while (message.times_processed.load(di::MemoryOrder::Acquire) != count) {
        di::cpu_relax();
    }

    // Return the message to the pool.
    iris::global_state().ipi_message_pool.lock()->deallocate(message);
}
}

namespace iris::x86::amd64 {
static IrqStatus handle_ipi_irq(IrqContext&) {
    // Acknowledge the interrupt before processing messages. This way, if we get more messages while processing, we'll
    // get another interrupt.
    // SAFETY: interrupts are disabled.
    auto& processor = current_processor_unsafe();
    processor.arch_processor().local_apic().send_eoi();

    processor.handle_pending_ipi_messages();
    return IrqStatus::Handled;
}

LocalApic::LocalApic(mm::PhysicalAddress base) {
    m_base = &mm::map_physical_address(base, 0x1000)->typed<u32 volatile>();
}

class LocalApicTimer {
private:
    friend di::StringView tag_invoke(di::Tag<timer_name>, LocalApicTimer const&) { return "APIC"_sv; }

    friend TimerCapabilities tag_invoke(di::Tag<timer_capabilities>, LocalApicTimer const&) {
        return TimerCapabilities::Periodic | TimerCapabilities::NeedsCalibration | TimerCapabilities::PerCpu;
    }

    friend TimerResolution tag_invoke(di::Tag<timer_resolution>, LocalApicTimer const& self) {
        return self.m_resolution;
    }

    friend Expected<void> tag_invoke(di::Tag<timer_set_interval>, LocalApicTimer& self, TimerResolution duration,
                                     di::Function<void(IrqContext&)> callback) {
        ASSERT(interrupts_disabled());

        // SAFETY: interrupts are disabled.
        auto& processor = current_processor_unsafe();
        auto& local_apic = processor.arch_processor().local_apic();

        processor.arch_processor().set_local_apic_callback(di::move(callback));

        // Calculate the number of ticks to count down from.
        auto ticks = duration / self.m_resolution;

        println("Setting APIC timer interval to {} ticks ({} ps)"_sv, ticks, duration.count());

        // Set the APIC timer to count down from the calculated number of ticks.
        local_apic.write_timer_divide_configuration(ApicTimerDivideConfiguration::DivideBy2);
        local_apic.write_lvt_entry(ApicOffset::TimerLvtEntry,
                                   ApicLvtEntry(ApicLvtEntryVector(62), ApicLvtEntryTimerMode(true)));
        local_apic.write_timer_initial_count(ticks);

        return {};
    }

    friend Expected<void> tag_invoke(di::Tag<timer_calibrate>, LocalApicTimer& self) {
        ASSERT(interrupts_disabled());

        // SAFETY: interrupts are disabled.
        auto& processor = current_processor_unsafe();
        auto& local_apic = processor.arch_processor().local_apic();
        auto& calibration_timer = iris::calibration_timer();

        // To calibrate the timer, we can use a reference timer with known accuracy. Then set it to send an interrupt
        // after 50 ms, and measure how many ticks elapsed on the APIC timer. Typically, the reference timer will be the
        // HPET, although for now the PIT is used.

        // Set the APIC timer to count down from 0xFFFFFFFF.
        local_apic.write_timer_divide_configuration(ApicTimerDivideConfiguration::DivideBy2);
        local_apic.write_lvt_entry(ApicOffset::TimerLvtEntry, ApicLvtEntry(ApicLvtEntryVector(62)));
        local_apic.write_timer_initial_count(0xFFFFFFFF);

        // Now set the reference timer to fire in 50 ms.
        auto new_ticks = 0xFFFFFFFF_u32;
        *timer_set_single_shot(*calibration_timer.lock(), 50_ms, [&](IrqContext&) {
            new_ticks = local_apic.timer_current_count();

            // Disable the APIC timer.
            local_apic.write_lvt_entry(ApicOffset::TimerLvtEntry,
                                       ApicLvtEntry(ApicLvtEntryVector(62), ApicLvtEntryMask(true)));
            local_apic.write_timer_current_count(0);
        });

        // Now wait for the calibration timer to fire.
        // FIXME: what if the timer never fires? It would be nice to have a timeout, but how can we do that if the only
        // timer available is broken...
        asm volatile("sti\n"
                     "hlt\n"
                     "cli\n" ::
                         : "memory");

        // Calculate the resolution.
        // NOTE: picoseconds are used because modern CPUs have a frequency larger than 1 GHz. They thus have
        // sub-nanosecond resolution.
        auto ticks = 0xFFFFFFFF_u32 - new_ticks;
        self.m_resolution = di::Picoseconds(50_ms) / ticks;

        println("APIC timer resolution: {} ps. {} ticks elapsed. Waited for {} ps."_sv, self.m_resolution.count(),
                ticks, di::Picoseconds(50_ms).count());

        return {};
    }

    di::Picoseconds m_resolution;
};

static_assert(di::Impl<LocalApicTimer, TimerInterface>);

static void init_local_apic_timer() {
    auto& global_state = global_state_in_boot();
    if (!global_state.arch_readonly_state.use_apic) {
        return;
    }

    *register_exception_handler(GlobalIrqNumber(62), [](IrqContext& context) -> IrqStatus {
        // SAFETY: interrupts are disabled.
        auto& processor = current_processor_unsafe();

        processor.arch_processor().local_apic().send_eoi();
        processor.arch_processor().local_apic_callback(context);

        return IrqStatus::Handled;
    });

    *global_state.timers.emplace_back(LocalApicTimer {});
}

void init_local_apic(bool print_info) {
    auto& global_state = global_state_in_boot();
    if (!global_state.arch_readonly_state.use_apic) {
        println("APIC support is disabled, so skipping local APIC initialization..."_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    if (!global_state.processor_info.has_apic()) {
        println("ACPI not detected, so skipping local APIC initialization..."_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    // Ensure the local APIC is enabled.
    auto apic_msr = read_msr(ModelSpecificRegister::LocalApicBase);
    apic_msr |= 0x800;
    write_msr(ModelSpecificRegister::LocalApicBase, apic_msr);

    auto apic_base = mm::PhysicalAddress(apic_msr & 0xFFFFF000);
    auto local_apic = LocalApic(apic_base);

    auto apic_id = local_apic.id();
    auto apic_version = local_apic.version();
    if (print_info) {
        println("Local APIC ID: {}"_sv, apic_id);
        println("Local APIC version: {}"_sv, apic_version.get<ApicVersion>());
        println("Local APIC max LVT entry: {}"_sv, apic_version.get<ApicMaxLvtEntry>());
        println("Local APIC EOI extended register present: {}"_sv, apic_version.get<ApicExtendedRegisterPresent>());

        // FIXME: The print_info parameter doubles as a "is_bsp" flag.
        // Register the IPI IRQ handler.
        *register_exception_handler(GlobalIrqNumber(63), handle_ipi_irq);

        // Setup the local APIC timer.
        init_local_apic_timer();
    }

    // Enable the local APIC by setting up the spurious interrupt vector register.
    // This also maps any spurious interrupts to IRQ 255.
    local_apic.write_spurious_interrupt_vector(0x1FF);

    // SAFETY: This is safe because this is called during processor boot.
    current_processor_unsafe().arch_processor().set_local_apic(local_apic);
}

struct ApBootInfo {
    u64 cr3;
    u64 stack_pointer;
    Processor* processor;
    GDTR gdtr;
    alignas(8) di::Array<sd::SegmentDescriptor, 11> gdt {};
};

asm(".code16\n"
    ".set __iris_ap_phys_start, 0x8000\n"
    ".global __iris_ap_entry_start\n"
    "__iris_ap_entry_start:\n"
    "cli\n"

    // Enable cr4.PAE and cr4.PGE
    "mov %cr4, %eax\n"
    "or $(1 << 5 | 1 << 7), %eax\n"
    "mov %eax, %cr4\n"

    // Enable EFER.LME and EFER.NXE
    "mov $0xC0000080, %ecx\n"
    "rdmsr\n"
    "or $(1 << 8 | 1 << 11), %eax\n"
    "wrmsr\n"

    // Load CR3
    "mov (__iris_ap_cr3 - __iris_ap_entry_start + __iris_ap_phys_start), %eax\n"
    "mov %eax, %cr3\n"

    // Enable long mode and paging by setting cr0.PG and cr0.PE
    "mov %cr0, %eax\n"
    "or $(1 << 31 | 1 << 0), %eax\n"
    "mov %eax, %cr0\n"

    // Load the GDT and jump to 64-bit mode.
    "lgdtl (__iris_ap_gdtr - __iris_ap_entry_start + __iris_ap_phys_start)\n"
    "mov $6, %ebx\n"
    "ljmp $0x28, $(__iris_ap_entry64 - __iris_ap_entry_start + __iris_ap_phys_start)\n"

    ".code64\n"
    "__iris_ap_entry64:\n"

    // Clear the segment registers
    "xor %rax, %rax\n"
    "mov %ax, %ds\n"
    "mov %ax, %es\n"
    "mov %ax, %fs\n"
    "mov %ax, %gs\n"
    "mov %ax, %ss\n"

    // Load the stack pointer
    "mov (__iris_ap_stack_pointer - __iris_ap_entry_start + __iris_ap_phys_start), %rsp\n"

    // Load the boot info pointer into %rdi
    "mov $(__iris_ap_boot_info - __iris_ap_entry_start + __iris_ap_phys_start), %rdi\n"

    // Jump to the C++ entry point
    "mov $iris_ap_entry, %rax\n"
    "call *%rax\n"

    // Loop forever
    "__iris_ap_loop:\n"
    "hlt\n"
    "jmp __iris_ap_loop\n"

    ".align 16\n"
    "__iris_ap_boot_info:\n"
    "__iris_ap_cr3:\n"
    ".skip 8\n"
    "__iris_ap_stack_pointer:\n"
    ".skip 8\n"
    "__iris_ap_processor:\n"
    ".skip 8\n"
    "__iris_ap_gdtr:\n"
    ".skip 16\n"
    "__iris_ap_gdt:\n"
    ".skip 88\n"

    "__iris_ap_entry_end:\n");

extern "C" {
extern void __iris_ap_entry_start();
extern void __iris_ap_boot_info();
extern void __iris_ap_gdt();
extern void __iris_ap_entry_end();
}

static inline mm::VirtualAddress ap_entry((u64) &__iris_ap_entry_start);
static inline mm::VirtualAddress ap_boot_info((u64) &__iris_ap_boot_info);
static inline mm::VirtualAddress ap_gdt((u64) &__iris_ap_gdt);
static inline mm::VirtualAddress ap_entry_end((u64) &__iris_ap_entry_end);

extern "C" void iris_ap_entry(ApBootInfo* info_in) {
    // Copy the boot info onto the new stack, since the info_in pointer refers to identity-mapped low memory.
    auto info = *info_in;

    info.processor->mark_as_booted();

    info.processor->arch_processor().enable_cpu_features(false);
    set_current_processor(*info.processor);

    println("AP {} booted."_sv, info.processor->id());

    idt::load_idt();
    init_tss();
    init_gdt();

    set_current_processor(*info.processor);

    info.processor->arch_processor().setup_fpu_support_for_processor(false);

    init_local_apic(false);

    println("AP {} initialized."_sv, info.processor->id());
    info.processor->mark_as_initialized();

    while (!global_state().all_aps_booted.load(di::MemoryOrder::Acquire)) {
        di::cpu_relax();
    }

    // Fully flush the tlb, since we weren't getting IPIs before this point.
    info.processor->flush_tlb_local();

    println("AP {} starting scheduler."_sv, info.processor->id());
    info.processor->scheduler().start_on_ap();
}

static void boot_ap(acpi::ProcessorLocalApicStructure const& acpi_local_apic_structure) {
    auto& global_state = global_state_in_boot();

    constexpr auto ap_stack_size = 0x2000_usize;

    auto stack_object = *di::make_arc<mm::BackingObject>();
    auto stack = *global_state.kernel_address_space.allocate_region(
        di::move(stack_object), ap_stack_size, mm::RegionFlags::Readable | mm::RegionFlags::Writable);

    auto& processor = *global_state.alernate_processors.emplace_back(acpi_local_apic_structure.apic_id);
    processor.arch_processor().set_fallback_kernel_stack((stack + ap_stack_size).raw_value());
    processor.scheduler().setup_idle_task();

    auto boot_info = ApBootInfo {
        .cr3 = global_state.kernel_address_space.architecture_page_table_base().raw_value(),
        .stack_pointer = stack.raw_value() + ap_stack_size,
        .processor = &processor,
        .gdtr = {},
        .gdt = global_state.boot_processor.arch_processor().gdt(),
    };

    auto boot_area_size = ap_entry_end - ap_entry;
    auto boot_info_offset = ap_boot_info - ap_entry;
    auto boot_gdt_offset = ap_gdt - ap_entry;

    boot_info.gdtr = { sizeof(boot_info.gdt) - 1, 0x8000 + usize(boot_gdt_offset) };

    ASSERT_LT_EQ(boot_area_size, 0x1000);

    *global_state.kernel_address_space.lock()->create_low_identity_mapping(mm::VirtualAddress(0x8000), 0x1000);

    // Setup the boot area.
    auto* boot_area = reinterpret_cast<byte*>(0x8000);
    di::copy(di::Span { reinterpret_cast<byte const*>(ap_entry.raw_value()), usize(boot_area_size) }, boot_area);
    di::copy(di::Span { reinterpret_cast<byte const*>(&boot_info), sizeof(boot_info) }, boot_area + boot_info_offset);

    // SAFETY: This is safe because we're still in boot.
    auto& local_apic = current_processor_unsafe().arch_processor().local_apic();

    // Send INIT IPI
    auto icr = ApicInterruptCommandRegister(ApicInterruptCommandDeliveryMode(ApicMessageType::Init),
                                            ApicInterruptCommandLevel(true),
                                            ApicInterruptCommandDestination(acpi_local_apic_structure.apic_id));

    println("Sending INIT IPI to AP {}..."_sv, acpi_local_apic_structure.apic_id);
    local_apic.write_interrupt_command_register(icr);

    io_wait_us(10000);

    // Send SIPI IPI
    icr.set<ApicInterruptCommandDeliveryMode>(ApicMessageType::Startup);
    icr.set<ApicInterruptCommandVector>(0x8000 >> 12);

    println("Sending SIPI IPI to AP {}..."_sv, acpi_local_apic_structure.apic_id);
    local_apic.write_interrupt_command_register(icr);

    for (u32 i = 0; i < 200; i++) {
        if (processor.is_booted()) {
            break;
        }
        io_wait();
    }

    if (!processor.is_booted()) {
        println("Sending SIPI IPI to AP {}..."_sv, acpi_local_apic_structure.apic_id);
        local_apic.write_interrupt_command_register(icr);
    }

    for (u32 i = 0; i < 1000000; i++) {
        if (processor.is_initialized()) {
            break;
        }
        io_wait();
    }

    if (!processor.is_initialized()) {
        println("AP {} failed to boot!"_sv, acpi_local_apic_structure.apic_id);
    }

    *global_state.processor_map.try_emplace(processor.id(), &processor);

    *global_state.kernel_address_space.lock()->remove_low_identity_mapping(mm::VirtualAddress(0x8000), 0x1000);
}

void init_alternative_processors() {
    auto& global_state = global_state_in_boot();
    if (!global_state.arch_readonly_state.use_apic) {
        println("APIC support is disabled, so skipping booting APs..."_sv);
        return;
    }

    *global_state.processor_map.try_emplace(global_state.boot_processor.id(), &global_state.boot_processor);

    auto message_pool = *ObjectPool<IpiMessage>::create(256);
    global_state.ipi_message_pool.get_assuming_no_concurrent_accesses() = di::move(message_pool);

    auto count = 0_u32;
    for (auto const& local_apic : global_state.acpi_info->local_apic) {
        if (local_apic.apic_id == global_state.boot_processor.arch_processor().local_apic().id()) {
            continue;
        }
        boot_ap(local_apic);
        count++;
    }

    println("Booted {} APs."_sv, count);
    if (count > 0) {
        global_state.all_aps_booted.store(true, di::MemoryOrder::Release);
    }
}
}
