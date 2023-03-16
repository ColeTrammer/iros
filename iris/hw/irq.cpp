#include <iris/arch/x86/amd64/hw/pic.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/userspace_access.h>
#include <iris/fs/initrd.h>
#include <iris/hw/irq.h>
#include <iris/hw/irq_controller.h>
#include <iris/hw/power.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<GlobalIrqNumber> irq_number_for_legacy_isa_interrupt_number(IrqLine irq_line) {
    return GlobalIrqNumber(irq_line.raw_value() + 32);
}

Expected<void> register_irq_handler(GlobalIrqNumber irq, IrqHandler handler) {
    auto& irq_handlers = global_state().irq_handlers;
    return irq_handlers.with_lock([&](auto& irq_handlers) -> Expected<void> {
        // FIXME: propogate allocation failures.
        irq_handlers[irq.raw_value()].push_back(di::move(handler));

        if (auto irq_controller = irq_controller_for_interrupt_number(irq)) {
            enable_irq_line(*irq_controller, irq);
        }
        return {};
    });
}

extern "C" void generic_irq_handler(GlobalIrqNumber irq, iris::arch::TaskState* task_state, int error_code) {
    if (irq == GlobalIrqNumber(32)) {
        send_eoi(global_state().irq_controller.get_assuming_no_concurrent_accesses(), GlobalIrqNumber(32));

        // If preemption is disabled, do not reshcedule the currently running task but let it know
        // that it should yield whenever it finally re-enables preemption.
        auto& current_task = iris::global_state().scheduler.current_task();
        if (current_task.preemption_disabled()) {
            current_task.set_should_be_preempted();
            return;
        }
        iris::global_state().scheduler.save_state_and_run_next(task_state);
    }

    if (irq == GlobalIrqNumber(36)) {
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

        send_eoi(global_state().irq_controller.get_assuming_no_concurrent_accesses(), GlobalIrqNumber(36));
        return;
    }

    if (irq == GlobalIrqNumber(0x80)) {
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
    ASSERT(false);
}
}
