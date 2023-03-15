#include <di/prelude.h>
#include <iris/arch/x86/amd64/hw/pic.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/core/userspace_access.h>
#include <iris/core/wait_queue.h>
#include <iris/fs/debug_file.h>
#include <iris/fs/file.h>
#include <iris/fs/initrd.h>
#include <iris/hw/power.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<usize> tag_invoke(di::Tag<read_file>, DebugFile&, di::Span<di::Byte> buffer) {
    if (buffer.empty()) {
        return 0;
    }

    auto byte = di::Byte(0);
    TRY(global_state().input_wait_queue.wait([&] {
        auto has_data = global_state().input_data_queue.pop();
        if (has_data) {
            byte = *has_data;
            return true;
        }
        return false;
    }));

    buffer[0] = byte;
    return 1;
}
}

namespace iris::arch {
[[noreturn]] static void done() {
    hard_shutdown(ShutdownStatus::Error);
    di::unreachable();
}

extern "C" void generic_irq_handler(int irq, iris::arch::TaskState* task_state, int error_code) {
    if (irq == 32) {
        iris::x86::amd64::send_eoi(0);

        // If preemption is disabled, do not reshcedule the currently running task but let it know
        // that it should yield whenever it finally re-enables preemption.
        auto& current_task = iris::global_state().scheduler.current_task();
        if (current_task.preemption_disabled()) {
            current_task.set_should_be_preempted();
            return;
        }
        iris::global_state().scheduler.save_state_and_run_next(task_state);
    }

    if (irq == 36) {
        while ((x86::amd64::io_in<u8>(0x3F8 + 5) & 1) == 0)
            ;

        auto byte = x86::amd64::io_in<di::Byte>(0x3F8);
        if (byte == '\r'_b) {
            byte = '\n'_b;
        }

        log_output_byte(byte);

        global_state().input_wait_queue.notify_one([&] {
            global_state().input_data_queue.push(byte);
        });

        iris::x86::amd64::send_eoi(4);
        return;
    }

    if (irq == 0x80) {
        // System call.
        auto number = SystemCall(task_state->rax);
        switch (number) {
            case SystemCall::debug_print: {
                auto string_base = task_state->rdi;
                auto string_length = task_state->rsi;
                auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

                iris::with_userspace_access([&] {
                    iris::print("{}"_sv, string);
                });
                break;
            }
            case SystemCall::shutdown: {
                iris::println("Shutdowning down..."_sv);
                auto success = task_state->rdi == 0;
                iris::hard_shutdown(success ? ShutdownStatus::Intended : ShutdownStatus::Error);
                break;
            }
            case SystemCall::exit_task: {
                iris::println("Exiting task..."_sv);

                iris::global_state().scheduler.exit_current_task();
                break;
            }
            case SystemCall::create_task: {
                auto& current_task = iris::global_state().scheduler.current_task();
                auto result = iris::create_user_task(current_task.task_namespace(), current_task.file_table());
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = (*result)->id().raw_value();
                }
                break;
            }
            case SystemCall::load_executable: {
                auto task_id = iris::TaskId(task_state->rdi);
                auto string_base = task_state->rsi;
                auto string_length = task_state->rdx;
                auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

                iris::with_userspace_access([&] {
                    auto path = di::PathView { string };
                    iris::println("Loading executable for {}: {}..."_sv, task_id, path);

                    auto& task_namespace = iris::global_state().scheduler.current_task().task_namespace();
                    auto result = task_namespace.lock()->find_task(task_id);
                    if (!result) {
                        task_state->rdx = di::bit_cast<u64>(result.error());
                        return;
                    }

                    auto result2 = iris::load_executable(*result, path);
                    if (!result2) {
                        task_state->rdx = di::bit_cast<u64>(result2.error());
                    } else {
                        task_state->rdx = 0;
                        task_state->rax = 0;
                    }
                });
                break;
            }
            case SystemCall::start_task: {
                auto task_id = iris::TaskId(task_state->rdi);

                auto& task_namespace = iris::global_state().scheduler.current_task().task_namespace();
                auto result = task_namespace.lock()->find_task(task_id);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = 0;
                }

                global_state().scheduler.schedule_task(*result);
                break;
            }
            case SystemCall::allocate_memory: {
                auto amount = task_state->rdi;

                auto& address_space = iris::global_state().scheduler.current_address_space();
                auto result = address_space.allocate_region(amount, mm::RegionFlags::User | mm::RegionFlags::Writable |
                                                                        mm::RegionFlags::Readable);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = result->raw_value();
                }
                break;
            }
            case SystemCall::open: {
                auto string_base = task_state->rdi;
                auto string_length = task_state->rsi;
                auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

                auto& current_task = iris::global_state().scheduler.current_task();

                iris::with_userspace_access([&] {
                    auto path = di::PathView { string };

                    auto result = current_task.file_table().allocate_file_handle();
                    if (!result) {
                        task_state->rdx = di::bit_cast<u64>(result.error());
                        return;
                    }

                    auto result2 = iris::open_in_initrd(path);
                    if (!result) {
                        task_state->rdx = di::bit_cast<u64>(result.error());
                    } else {
                        di::get<0>(*result) = *di::move(result2);
                        task_state->rdx = 0;
                        task_state->rax = di::get<1>(*result);
                    }
                });
                break;
            }
            case SystemCall::write: {
                i32 file_handle = task_state->rdi;
                auto buffer = reinterpret_cast<di::Byte const*>(task_state->rsi);
                auto amount = task_state->rdx;

                auto& current_task = iris::global_state().scheduler.current_task();
                auto result = current_task.file_table().lookup_file_handle(file_handle);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                    break;
                }

                auto result2 = iris::with_userspace_access([&] {
                    return iris::write_file(*result, { buffer, amount });
                });
                if (!result2) {
                    task_state->rdx = di::bit_cast<u64>(result2.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = *result2;
                }
                break;
            }
            case SystemCall::read: {
                i32 file_handle = task_state->rdi;
                auto buffer = reinterpret_cast<di::Byte*>(task_state->rsi);
                auto amount = task_state->rdx;

                auto& current_task = iris::global_state().scheduler.current_task();
                auto result = current_task.file_table().lookup_file_handle(file_handle);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                    break;
                }

                auto result2 = iris::with_userspace_access([&] {
                    return iris::read_file(*result, { buffer, amount });
                });
                if (!result2) {
                    task_state->rdx = di::bit_cast<u64>(result2.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = *result2;
                }
                break;
            }
            case SystemCall::close: {
                i32 file_handle = task_state->rdi;

                auto& current_task = iris::global_state().scheduler.current_task();
                auto result = current_task.file_table().deallocate_file_handle(file_handle);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = 0;
                }
                break;
            }
            case SystemCall::start_task_and_block: {
                auto task_id = iris::TaskId(task_state->rdi);

                auto& task_namespace = iris::global_state().scheduler.current_task().task_namespace();
                auto result = task_namespace.lock()->find_task(task_id);
                if (!result) {
                    task_state->rdx = di::bit_cast<u64>(result.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = 0;
                }

                global_state().scheduler.schedule_task(*result);

                auto task_status = result->task_status();
                auto result2 = task_status->wait_until_exited();
                if (!result2) {
                    task_state->rdx = di::bit_cast<u64>(result2.error());
                } else {
                    task_state->rdx = 0;
                    task_state->rax = 0;
                }
                break;
            }
            default:
                iris::println("Encounted unexpected system call: {}"_sv, di::to_underlying(number));

                task_state->rax = 0;
                task_state->rdx = di::bit_cast<u64>(Error::NotSupported);
                break;
        }
        return;
    }

    iris::println("ERROR: got unexpected IRQ {}, error_code={}, ip={:#x}"_sv, irq, error_code, task_state->rip);
    done();
}

template<int irq_number>
[[gnu::naked]] [[noreturn]] static void handle_irq_without_error_code() {
    asm volatile("push %%rax\n"
                 "push %%rbx\n"
                 "push %%rcx\n"
                 "push %%rdx\n"
                 "push %%rsi\n"
                 "push %%rdi\n"
                 "push %%rbp\n"
                 "push %%r8\n"
                 "push %%r9\n"
                 "push %%r10\n"
                 "push %%r11\n"
                 "push %%r12\n"
                 "push %%r13\n"
                 "push %%r14\n"
                 "push %%r15\n "

                 "mov %0, %%rdi\n"
                 "mov %%rsp, %%rsi\n"
                 "xor %%rdx, %%rdx\n"
                 "callq generic_irq_handler\n"

                 "pop %%r15\n"
                 "pop %%r14\n"
                 "pop %%r13\n"
                 "pop %%r12\n"
                 "pop %%r11\n"
                 "pop %%r10\n"
                 "pop %%r9\n"
                 "pop %%r8\n"
                 "pop %%rbp\n"
                 "pop %%rdi\n"
                 "pop %%rsi\n"
                 "pop %%rdx\n"
                 "pop %%rcx\n"
                 "pop %%rbx\n"
                 "pop %%rax\n"

                 "iretq\n"
                 :
                 : "i"(irq_number));
}

template<int irq_number>
[[gnu::naked]] [[noreturn]] static void handle_irq_with_error_code() {
    asm volatile("xor (%%rsp), %%rax\n"
                 "xor %%rax, (%%rsp)\n"
                 "xor (%%rsp), %%rax\n"

                 "push %%rbx\n"
                 "push %%rcx\n"
                 "push %%rdx\n"
                 "push %%rsi\n"
                 "push %%rdi\n"
                 "push %%rbp\n"
                 "push %%r8\n"
                 "push %%r9\n"
                 "push %%r10\n"
                 "push %%r11\n"
                 "push %%r12\n"
                 "push %%r13\n"
                 "push %%r14\n"
                 "push %%r15\n "

                 "mov $4, %%rdi\n"
                 "mov %%rsp, %%rsi\n"
                 "mov %%rax, %%rdx\n"
                 "callq generic_irq_handler\n"

                 "pop %%r15\n"
                 "pop %%r14\n"
                 "pop %%r13\n"
                 "pop %%r12\n"
                 "pop %%r11\n"
                 "pop %%r10\n"
                 "pop %%r9\n"
                 "pop %%r8\n"
                 "pop %%rbp\n"
                 "pop %%rdi\n"
                 "pop %%rsi\n"
                 "pop %%rdx\n"
                 "pop %%rcx\n"
                 "pop %%rbx\n"
                 "pop %%rax\n"

                 "iretq\n"
                 :
                 : "i"(irq_number));
}

template<int irq_number>
constexpr auto get_irq_handler() {
    // For a list of x86_64 IRQ with push an error code onto the stack, see the exception table
    // on the OSDEV wiki: https://wiki.osdev.org/Exceptions.
    constexpr auto exceptions_with_error_code = di::Array { 8, 10, 11, 12, 13, 14, 17, 21, 28, 29, 30 };

    if constexpr (di::contains(exceptions_with_error_code, irq_number)) {
        return handle_irq_with_error_code<irq_number>;
    } else {
        return handle_irq_without_error_code<irq_number>;
    }
}

static char temp_stack[4 * 4096] alignas(4096);

static auto idt = di::Array<iris::x86::amd64::idt::Entry, 256> {};
static auto gdt = di::Array<iris::x86::amd64::sd::SegmentDescriptor, 11> {};
static auto tss = iris::x86::amd64::TSS {};

void load_kernel_stack(mm::VirtualAddress base) {
    tss.rsp[0] = base.raw_value();
}

extern "C" void bsp_cpu_init() {
    iris::arch::cxx_init();

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

    auto& global_state = global_state_in_boot();

    iris::println("Beginning x86_64 kernel boot..."_sv);

    global_state.processor_info = detect_processor_info();
    global_state.processor_info.print_to_console();

    if (!!(global_state.processor_info.features & ProcessorFeatures::Smep)) {
        iris::println("Enabling SMEP..."_sv);
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 20));
    }

    if (!!(global_state.processor_info.features & ProcessorFeatures::Smap)) {
        iris::println("Enabling SMAP..."_sv);
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 21));
    }

    {
        using namespace iris::x86::amd64::idt;

        di::function::unpack<di::meta::MakeIntegerSequence<int, 256>>(
            []<int... indices>(di::meta::IntegerSequence<int, indices...>) {
                auto do_entry = []<int n>(di::Nontype<n>) {
                    auto handler_address = di::to_uintptr(get_irq_handler<n>());
                    idt[n] = Entry(Present(true), Type(Type::InterruptGate), SegmentSelector(5 * 8),
                                   TargetLow(handler_address & 0xFFFF), TargetMid((handler_address >> 16) & 0xFFFF),
                                   TargetHigh(handler_address >> 32), DPL(3));
                };

                (do_entry(di::nontype<indices>), ...);
            });

        auto idtr = iris::x86::amd64::IDTR { sizeof(idt) - 1, di::to_uintptr(idt.data()) };
        iris::x86::amd64::load_idt(idtr);
    }

    {
        using namespace iris::x86::amd64::ssd;

        // Setup TSS.
        tss.io_map_base = sizeof(tss);
        tss.rsp[0] = di::to_uintptr(temp_stack);
        tss.rsp[1] = di::to_uintptr(temp_stack);
        tss.rsp[2] = di::to_uintptr(temp_stack);
        tss.ist[0] = di::to_uintptr(temp_stack);

        // TSS Descriptor Setup.
        auto tss_descriptor = reinterpret_cast<SystemSegmentDescriptor*>(&gdt[9]);
        auto tss_address = di::to_uintptr(&tss);
        *tss_descriptor = SystemSegmentDescriptor(
            LimitLow(sizeof(tss)), BaseLow(tss_address & 0xFFFF), BaseMidLow((tss_address >> 16) & 0xFF),
            Type(Type::TSS), Present(true), BaseMidHigh((tss_address >> 24) & 0xFF), BaseHigh((tss_address >> 32)));
    }

    {
        using namespace iris::x86::amd64::sd;

        // The layout of the GDT matches the limine boot protocol, although this is not strictly necessary.
        // The 16 bit and 32 bit segments are included to ease future attempts to boot APs.
        // This is the null segment descriptor.
        gdt[0] = SegmentDescriptor();

        // 16 bit Code Descriptor.
        gdt[1] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 16 bit Data Descriptor.
        gdt[2] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 32 bit Code Descriptor.
        gdt[3] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 32 bit Data Descriptor.
        gdt[4] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 64 bit Code Descriptor.
        gdt[5] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit Data Descriptor.
        gdt[6] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 64 bit User Code Descriptor.
        gdt[7] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), DPL(3),
                                   Present(true), LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit User Data Descriptor.
        gdt[8] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), DPL(3), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        auto gdtr = iris::x86::amd64::GDTR { sizeof(gdt) - 1, di::to_uintptr(gdt.data()) };
        iris::x86::amd64::load_gdt(gdtr);

        // Load TSS.
        iris::x86::amd64::load_tr(9 * 8);

        // Load the data segments with NULL segment selector.
        asm volatile("mov %0, %%dx\n"
                     "mov %%dx, %%ds\n"
                     "mov %%dx, %%es\n"
                     "mov %%dx, %%fs\n"
                     "mov %%dx, %%ss\n"
                     "mov %%dx, %%gs\n"
                     :
                     : "i"(0)
                     : "memory", "edx");
    }

    iris::x86::amd64::init_pic();

    iris_main();
}

extern "C" [[gnu::naked]] void iris_entry() {
    asm volatile("mov %0, %%rsp\n"
                 "push $0\n"
                 "call bsp_cpu_init\n"
                 :
                 : "r"(temp_stack + sizeof(temp_stack))
                 : "memory");
}
}
