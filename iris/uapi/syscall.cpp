#include <iris/core/print.h>
#include <iris/core/task.h>
#include <iris/core/userspace_access.h>
#include <iris/fs/initrd.h>
#include <iris/hw/power.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<u64> do_syscall(Task& current_task, arch::TaskState& task_state) {
    auto number = task_state.syscall_number();
    switch (number) {
        case SystemCall::debug_print: {
            auto string_base = task_state.syscall_arg1();
            auto string_length = task_state.syscall_arg2();
            auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

            iris::with_userspace_access([&] {
                iris::print("{}"_sv, string);
            });
            return 0;
        }
        case SystemCall::shutdown: {
            iris::println("Shutdowning down..."_sv);
            auto success = task_state.syscall_arg1() == 0;
            iris::hard_shutdown(success ? ShutdownStatus::Intended : ShutdownStatus::Error);
            break;
        }
        case SystemCall::exit_task: {
            iris::println("Exiting task..."_sv);

            iris::global_state().scheduler.exit_current_task();
            break;
        }
        case SystemCall::create_task: {
            auto task = TRY(iris::create_user_task(current_task.task_namespace(), current_task.file_table(),
                                                   current_task.address_space().arc_from_this()));
            return task->id().raw_value();
        }
        case SystemCall::load_executable: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto string_base = task_state.syscall_arg2();
            auto string_length = task_state.syscall_arg3();
            auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

            return iris::with_userspace_access([&] -> Expected<u64> {
                auto path = di::PathView { string };
                iris::println("Loading executable for {}: {}..."_sv, task_id, path);

                auto& task_namespace = iris::global_state().scheduler.current_task().task_namespace();
                auto task = TRY(task_namespace.lock()->find_task(task_id));

                TRY(iris::load_executable(*task, path));
                return 0;
            });
        }
        case SystemCall::start_task: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());

            auto& task_namespace = current_task.task_namespace();
            auto task = TRY(task_namespace.lock()->find_task(task_id));
            global_state().scheduler.schedule_task(*task);
            return 0;
        }
        case SystemCall::allocate_memory: {
            auto amount = task_state.syscall_arg1();

            auto& address_space = current_task.address_space();
            return address_space
                .allocate_region(amount, mm::RegionFlags::User | mm::RegionFlags::Writable | mm::RegionFlags::Readable)
                .transform(&mm::VirtualAddress::raw_value);
        }
        case SystemCall::open: {
            auto string_base = task_state.syscall_arg1();
            auto string_length = task_state.syscall_arg2();
            auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };

            auto& current_task = iris::global_state().scheduler.current_task();

            return iris::with_userspace_access([&] -> Expected<u64> {
                auto path = di::PathView { string };

                auto [file_storage, fd] = TRY(current_task.file_table().allocate_file_handle());

                auto file = TRY(iris::open_in_initrd(path));
                file_storage = di::move(file);

                return fd;
            });
        }
        case SystemCall::write: {
            i32 file_handle = task_state.syscall_arg1();
            auto buffer = reinterpret_cast<di::Byte const*>(task_state.syscall_arg2());
            auto amount = task_state.syscall_arg3();

            auto& current_task = iris::global_state().scheduler.current_task();
            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return iris::with_userspace_access([&] {
                return iris::write_file(handle, { buffer, amount });
            });
        }
        case SystemCall::read: {
            i32 file_handle = task_state.syscall_arg1();
            auto buffer = reinterpret_cast<di::Byte*>(task_state.syscall_arg2());
            auto amount = task_state.syscall_arg3();

            auto& current_task = iris::global_state().scheduler.current_task();
            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return iris::with_userspace_access([&] {
                return iris::read_file(handle, { buffer, amount });
            });
        }
        case SystemCall::close: {
            i32 file_handle = task_state.syscall_arg1();

            TRY(current_task.file_table().deallocate_file_handle(file_handle));
            return 0;
        }
        case SystemCall::start_task_and_block: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());

            auto& task_namespace = current_task.task_namespace();
            auto task = TRY(task_namespace.lock()->find_task(task_id));

            global_state().scheduler.schedule_task(*task);

            auto task_status = task->task_status();
            TRY(task_status->wait_until_exited());
            return 0;
        }
        case SystemCall::set_userspace_thread_pointer: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto value = task_state.syscall_arg2();

            auto& task_namespace = current_task.task_namespace();
            auto task = task_id == iris::TaskId(0) ? current_task.arc_from_this()
                                                   : TRY(task_namespace.lock()->find_task(task_id));
            task->set_userspace_thread_pointer(value);

            arch::load_userspace_thread_pointer(value);
            return 0;
        }
        case SystemCall::set_userspace_stack_pointer: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto value = task_state.syscall_arg2();

            auto& task_namespace = current_task.task_namespace();
            auto task = task_id == iris::TaskId(0) ? current_task.arc_from_this()
                                                   : TRY(task_namespace.lock()->find_task(task_id));
            task->set_stack_pointer(mm::VirtualAddress(value));
            return 0;
        }
        case SystemCall::set_userspace_instruction_pointer: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto value = task_state.syscall_arg2();

            auto& task_namespace = current_task.task_namespace();
            auto task = task_id == iris::TaskId(0) ? current_task.arc_from_this()
                                                   : TRY(task_namespace.lock()->find_task(task_id));
            task->set_instruction_pointer(mm::VirtualAddress(value));
            return 0;
        }
        case SystemCall::set_userspace_argument1: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto value = task_state.syscall_arg2();

            auto& task_namespace = current_task.task_namespace();
            auto task = task_id == iris::TaskId(0) ? current_task.arc_from_this()
                                                   : TRY(task_namespace.lock()->find_task(task_id));
            task->set_argument1(value);
            return 0;
        }
        default:
            iris::println("Encounted unexpected system call: {}"_sv, di::to_underlying(number));
            break;
    }
    return di::Unexpected(Error::OperationNotSupported);
}
}
