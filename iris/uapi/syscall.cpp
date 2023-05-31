#include <di/execution/algorithm/sync_wait.h>
#include <iris/core/print.h>
#include <iris/core/task.h>
#include <iris/core/userspace_access.h>
#include <iris/core/userspace_ptr.h>
#include <iris/fs/initrd.h>
#include <iris/fs/path.h>
#include <iris/hw/power.h>
#include <iris/uapi/metadata.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<u64> do_syscall(Task& current_task, arch::TaskState& task_state) {
    auto number = task_state.syscall_number();
    switch (number) {
        case SystemCall::debug_print: {
            auto const* string_base = reinterpret_cast<byte const*>(task_state.syscall_arg1());
            auto string_length = task_state.syscall_arg2();
            auto string_buffer = TRY(di::create<UserspaceBuffer>(string_base, string_length));
            auto string = TRY(string_buffer.copy_to_string());

            iris::print("{}"_sv, string);
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

            current_scheduler()->exit_current_task();
            break;
        }
        case SystemCall::create_task: {
            auto task = TRY(iris::create_user_task(current_task.task_namespace(), current_task.root_tnode(),
                                                   current_task.cwd_tnode(), current_task.file_table(),
                                                   current_task.address_space().arc_from_this()));
            return task->id().raw_value();
        }
        case SystemCall::load_executable: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto const* string_base = reinterpret_cast<byte const*>(task_state.syscall_arg2());
            auto string_length = task_state.syscall_arg3();
            auto string_buffer = TRY(di::create<UserspaceBuffer>(string_base, string_length));
            auto path = TRY(string_buffer.copy_to_path());

            iris::println("Loading executable for {}: {}..."_sv, task_id, path);

            auto& task_namespace = current_task.task_namespace();
            auto task = TRY(task_namespace.lock()->find_task(task_id));

            TRY(iris::load_executable(*task, path));
            return 0;
        }
        case SystemCall::start_task: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());

            auto& task_namespace = current_task.task_namespace();
            auto task = TRY(task_namespace.lock()->find_task(task_id));

            schedule_task(*task);
            return 0;
        }
        case SystemCall::allocate_memory: {
            auto amount = task_state.syscall_arg1();

            auto new_object = TRY(di::make_arc<mm::BackingObject>());
            auto& address_space = current_task.address_space();
            return address_space
                .allocate_region(di::move(new_object), amount,
                                 mm::RegionFlags::User | mm::RegionFlags::Writable | mm::RegionFlags::Readable)
                .transform(&mm::VirtualAddress::raw_value);
        }
        case SystemCall::open: {
            auto const* string_base = reinterpret_cast<byte const*>(task_state.syscall_arg1());
            auto string_length = task_state.syscall_arg2();
            auto mode = OpenMode(task_state.syscall_arg3());
            if (!!(mode & ~(OpenMode::Mask))) {
                return di::Unexpected(Error::InvalidArgument);
            }

            auto string_buffer = TRY(di::create<UserspaceBuffer>(string_base, string_length));
            auto path = TRY(string_buffer.copy_to_path());

            println("Opening {}"_sv, path);

            auto [file_storage, fd] = TRY(current_task.file_table().allocate_file_handle());

            auto file = TRY(iris::open_path(current_task.root_tnode(), current_task.cwd_tnode(), path, mode));
            file_storage = di::move(file);

            return fd;
        }
        case SystemCall::write: {
            auto file_handle = i32(task_state.syscall_arg1());
            auto const* buffer = reinterpret_cast<di::Byte const*>(task_state.syscall_arg2());
            auto amount = task_state.syscall_arg3();

            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return iris::write_file(handle, TRY(di::create<UserspaceBuffer>(buffer, amount)));
        }
        case SystemCall::read: {
            auto file_handle = i32(task_state.syscall_arg1());
            auto* buffer = reinterpret_cast<di::Byte*>(task_state.syscall_arg2());
            auto amount = task_state.syscall_arg3();

            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return TRY_UNERASE_ERROR(
                di::execution::sync_wait(iris::read_file(handle, TRY(di::create<UserspaceBuffer>(buffer, amount)))));
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

            schedule_task(*task);

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

            // NOTE: the userspace thread pointer gets loaded automatically when context switching back to userspace.
            task->set_userspace_thread_pointer(value);
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
        case SystemCall::lseek: {
            auto file_handle = int(task_state.syscall_arg1());
            auto offset = i64(task_state.syscall_arg2());
            auto whence = int(task_state.syscall_arg3());

            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return iris::seek_file(handle, offset, whence);
        }
        case SystemCall::set_task_arguments: {
            auto task_id = iris::TaskId(task_state.syscall_arg1());
            auto const* argument_array =
                reinterpret_cast<UserspaceBuffer<byte const> const*>(task_state.syscall_arg2());
            auto argument_count = task_state.syscall_arg3();
            auto const* enviornment_array =
                reinterpret_cast<UserspaceBuffer<byte const> const*>(task_state.syscall_arg4());
            auto enviornment_count = task_state.syscall_arg5();

            auto userspace_arguments = TRY(di::create<UserspaceBuffer>(argument_array, argument_count));
            auto userspace_enviornment = TRY(di::create<UserspaceBuffer>(enviornment_array, enviornment_count));

            auto& task_namespace = current_task.task_namespace();
            auto task = task_id == iris::TaskId(0) ? current_task.arc_from_this()
                                                   : TRY(task_namespace.lock()->find_task(task_id));

            auto arguments = di::Vector<di::TransparentString> {};
            TRY(userspace_arguments.copy_in_chunks<16>(
                [&](di::Span<UserspaceBuffer<byte const>> chunk) -> Expected<void> {
                    for (auto string : chunk) {
                        auto owned_string = TRY(string.copy_to_string());
                        TRY(arguments.push_back(di::move(owned_string)));
                    }
                    return {};
                }));

            auto enviornment = di::Vector<di::TransparentString> {};
            TRY(userspace_enviornment.copy_in_chunks<16>(
                [&](di::Span<UserspaceBuffer<byte const>> chunk) -> Expected<void> {
                    for (auto string : chunk) {
                        auto owned_string = TRY(string.copy_to_string());
                        TRY(enviornment.push_back(di::move(owned_string)));
                    }
                    return {};
                }));

            auto task_arguments = TRY(di::make_arc<TaskArguments>(di::move(arguments), di::move(enviornment)));
            task->set_task_arguments(di::move(task_arguments));

            return 0;
        }
        case SystemCall::path_metadata: {
            auto const* string_base = reinterpret_cast<byte const*>(task_state.syscall_arg1());
            auto string_length = task_state.syscall_arg2();
            auto string_buffer = TRY(di::create<UserspaceBuffer>(string_base, string_length));
            auto path = TRY(string_buffer.copy_to_path());

            auto metadata_ptr =
                TRY(di::create<UserspacePtr<Metadata>>(reinterpret_cast<Metadata*>(task_state.syscall_arg3())));

            auto file = TRY(iris::open_path(current_task.root_tnode(), current_task.cwd_tnode(), path, OpenMode::None));
            auto metadata = TRY(iris::file_metadata(file));
            TRY(metadata_ptr.write(metadata));
            return 0;
        }
        case SystemCall::read_directory: {
            auto file_handle = i32(task_state.syscall_arg1());
            auto* buffer = reinterpret_cast<di::Byte*>(task_state.syscall_arg2());
            auto amount = task_state.syscall_arg3();

            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));

            return iris::read_directory(handle, TRY(di::create<UserspaceBuffer>(buffer, amount)));
        }
        case SystemCall::truncate: {
            auto file_handle = i32(task_state.syscall_arg1());
            auto length = u64(task_state.syscall_arg2());

            auto& handle = TRY(current_task.file_table().lookup_file_handle(file_handle));
            return file_truncate(handle, length) % di::function::value(0);
        }
        case SystemCall::create_node: {
            auto const* string_base = reinterpret_cast<byte const*>(task_state.syscall_arg1());
            auto string_length = task_state.syscall_arg2();
            auto type = static_cast<MetadataType>(task_state.syscall_arg3());
            if (type != MetadataType::Regular && type != MetadataType::Directory) {
                return di::Unexpected(Error::InvalidArgument);
            }

            auto string_buffer = TRY(di::create<UserspaceBuffer>(string_base, string_length));
            auto path = TRY(string_buffer.copy_to_path());
            return create_node(current_task.root_tnode(), current_task.cwd_tnode(), path, type) %
                   di::function::value(0);
        }
        default:
            iris::println("Encounted unexpected system call: {}"_sv, di::to_underlying(number));
            break;
    }
    return di::Unexpected(Error::OperationNotSupported);
}
}
