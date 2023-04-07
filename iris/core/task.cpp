#include <iris/core/error.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/task.h>
#include <iris/core/task_namespace.h>
#include <iris/core/userspace_access.h>
#include <iris/fs/initrd.h>

namespace iris {
Task::Task(bool userspace, di::Arc<mm::AddressSpace> address_space, di::Arc<TaskNamespace> task_namespace, TaskId id,
           FileTable file_table, di::Arc<TaskStatus> task_status)
    : m_task_state(userspace)
    , m_address_space(di::move(address_space))
    , m_task_namespace(di::move(task_namespace))
    , m_task_status(di::move(task_status))
    , m_file_table(di::move(file_table))
    , m_id(id) {
    // Leak 1 reference count.
    (void) arc_from_this().release();
}

Task::~Task() {
    global_state().task_finalization_wait_queue.notify_one([&] {
        ASSERT(global_state().task_finalization_data_queue.push({ di::move(m_address_space), m_kernel_stack }));
    });
}

Expected<di::Arc<Task>> create_kernel_task(TaskNamespace& task_namespace, void (*entry)()) {
    auto entry_address = mm::VirtualAddress(di::to_uintptr(entry));

    auto& address_space = global_state().kernel_address_space;
    auto stack = TRY(address_space.allocate_region(0x2000, mm::RegionFlags::Readable | mm::RegionFlags::Writable));

    auto task_status = TRY(di::try_make_arc<TaskStatus>());
    auto task_id = TRY(task_namespace.lock()->allocate_task_id());
    auto result = TRY(di::try_make_arc<Task>(false, address_space.arc_from_this(), task_namespace.arc_from_this(),
                                             task_id, FileTable {}, di::move(task_status)));
    result->set_instruction_pointer(entry_address);
    result->set_stack_pointer(stack + 0x2000);
    result->set_kernel_stack(stack);
    TRY(task_namespace.lock()->register_task(*result));
    return result;
}

Expected<di::Arc<Task>> create_user_task(TaskNamespace& task_namespace, FileTable file_table,
                                         di::Arc<mm::AddressSpace> address_space) {
    auto task_status = TRY(di::try_make_arc<TaskStatus>());
    auto task_id = TRY(task_namespace.lock()->allocate_task_id());
    auto result = TRY(di::try_make_arc<Task>(true, di::move(address_space), task_namespace.arc_from_this(), task_id,
                                             di::move(file_table), di::move(task_status)));
    TRY(result->fpu_state().setup_fpu_state());

    auto kernel_stack = TRY(global_state().kernel_address_space.allocate_region(0x2000, mm::RegionFlags::Writable |
                                                                                            mm::RegionFlags::Readable));
    result->set_kernel_stack(kernel_stack);

    TRY(task_namespace.lock()->register_task(*result));
    return result;
}

Expected<void> load_executable(Task& task, di::PathView path) {
    auto raw_data = TRY(lookup_in_initrd(path));

    // Setup program stack.
    constexpr auto stack_size = 0x10000_usize;

    task.set_address_space(TRY(mm::create_empty_user_address_space()));
    auto user_stack = TRY(task.address_space().allocate_region(
        stack_size, mm::RegionFlags::Writable | mm::RegionFlags::User | mm::RegionFlags::Readable));

    using ElfHeader = di::exec::ElfHeader<>;
    using ProgramHeader = di::exec::ElfProgramHeader<>;
    using ProgramHeaderType = di::exec::ElfProgramHeaderType;

    auto* elf_header = raw_data.typed_pointer_unchecked<ElfHeader>(0);
    ASSERT_EQ(sizeof(ProgramHeader), elf_header->program_entry_size);

    // FIXME: consider disabling preemption instead.
    auto address_space = task.address_space().lock();
    TRY(with_interrupts_disabled([&] -> Expected<void> {
        auto& current_address_space = current_scheduler()->current_address_space();
        address_space->base().load();

        auto program_headers = raw_data.typed_span_unchecked<ProgramHeader>(elf_header->program_table_off,
                                                                            elf_header->program_entry_count);
        // FIXME: handle different program header types and memory protection.
        for (auto& program_header : program_headers) {
            // PT_LOAD
            if (program_header.type != ProgramHeaderType::Load) {
                continue;
            }

            auto end = program_header.virtual_addr.value() + program_header.memory_size;
            auto aligned_end = di::align_up(end, 4096);
            auto aligned_start = di::align_down(program_header.virtual_addr.value(), 4096);
            auto region = di::try_box<mm::Region>(mm::VirtualAddress(aligned_start), aligned_end - aligned_start,
                                                  mm::RegionFlags::User | mm::RegionFlags::Readable |
                                                      mm::RegionFlags::Executable | mm::RegionFlags::Writable);
            (void) address_space->allocate_region_at(*di::move(region));

            auto data = di::Span { reinterpret_cast<di::Byte*>(program_header.virtual_addr.value()),
                                   program_header.memory_size };

            auto zero_fill = program_header.memory_size - program_header.file_size;
            with_userspace_access([&] {
                di::copy(*raw_data.subspan(program_header.offset, program_header.file_size), data.data());
                di::fill_n(data.data() + program_header.file_size, zero_fill, 0_b);
            });
        }

        // Write task arguments into the user stack.
        auto task_arguments = task.task_arguments();
        if (!task_arguments) {
            // Pass no arguments and enviornment.
            task.set_argument1(0);
            task.set_argument2(0);
            task.set_argument3(0);
            task.set_argument4(0);

            // Ensure the stack is 16-byte aligned.
            task.set_stack_pointer(user_stack + stack_size - 16 + sizeof(uptr));
        } else {
            // Determine the amount of stack memory needed, and error if there is not at least 4096 bytes left.
            auto string_bytes_needed = di::concat(task_arguments->arguments(), task_arguments->enviornment()) |
                                       di::transform([&](di::TransparentString const& value) {
                                           // Remember to include the null terminator.
                                           return value.size_bytes() + 1;
                                       }) |
                                       di::sum;
            string_bytes_needed = di::align_up(string_bytes_needed, 16);

            auto argc = task_arguments->arguments().size();
            auto envc = task_arguments->enviornment().size();

            auto string_record_bytes_needed = (argc + envc) * sizeof(di::Span<char const>);
            auto bytes_needed = string_bytes_needed + string_record_bytes_needed + 16;
            if (bytes_needed > stack_size - 0x1000) {
                return di::Unexpected(Error::ArgumentListTooLong);
            }

            auto string_data_base = user_stack + stack_size - long(string_bytes_needed);
            auto string_record_base = string_data_base - long(string_record_bytes_needed);

            with_userspace_access([&] {
                // Write the strings and string records into the user stack.
                auto* string_data = string_data_base.typed_pointer<char>();
                auto* string_record_data = string_record_base.typed_pointer<di::Span<char const>>();
                for (auto const& string : di::concat(task_arguments->arguments(), task_arguments->enviornment())) {
                    // Write the string record.
                    *string_record_data++ = { string_data, string.size() };

                    // Write the string data, including the null terminator.
                    string_data = di::copy(di::Span { string.data(), string.size() + 1 }, string_data).out;
                }
            });

            // Set the task arguments.
            task.set_argument1(string_record_base.raw_value());
            task.set_argument2(argc);
            task.set_argument3(string_record_base.raw_value() + argc * sizeof(di::Span<char const>));
            task.set_argument4(envc);

            // Ensure the stack is 16-byte aligned.
            task.set_stack_pointer(string_record_base - 16 + sizeof(uptr));
        }
        current_address_space.load();
        return {};
    }));

    task.set_instruction_pointer(mm::VirtualAddress(elf_header->entry));
    return {};
}

void Task::enable_preemption() {
    auto count = m_preemption_disabled_count.load(di::MemoryOrder::Relaxed);
    ASSERT_GT(count, 0);

    bool should_yield = m_should_be_preempted.load(di::MemoryOrder::Relaxed) && count == 1;
    if (should_yield) {
        raw_disable_interrupts();
        m_should_be_preempted.store(false, di::MemoryOrder::Relaxed);
    }

    m_preemption_disabled_count.store(count - 1, di::MemoryOrder::Relaxed);

    if (should_yield) {
        // SAFETY: this is safe because interrupts are disabled.
        current_processor_unsafe().scheduler().yield();
    }
}
}
