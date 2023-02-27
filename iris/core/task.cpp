#include <iris/core/error.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/task.h>
#include <iris/core/task_namespace.h>
#include <iris/core/userspace_access.h>
#include <iris/fs/initrd.h>

namespace elf64 {
using Addr = u64;
using Off = i64;
using Half = u16;
using Word = u32;
using Sword = i32;
using Xword = u64;
using Sxword = i64;
using Byte = di::Byte;

struct ElfHeader {
    Byte ident[16];
    Half type;
    Half machine;
    Word version;
    Addr entry;
    Off program_table_off;
    Off section_table_off;
    Word flags;
    Half elf_header_size;
    Half program_entry_size;
    Half program_entry_count;
    Half section_entry_size;
    Half section_entry_count;
    Half string_table_section_index;
};

struct ProgramHeader {
    Word type;
    Word flags;
    Off offset;
    Addr virtual_addr;
    Addr physical_addr;
    Xword file_size;
    Xword memory_size;
    Xword align;
};
}

namespace iris {
Task::Task(mm::VirtualAddress entry, mm::VirtualAddress stack, bool userspace, di::Arc<mm::AddressSpace> address_space,
           di::Arc<TaskNamespace> task_namespace, TaskId id, FileTable file_table)
    : m_task_state(entry.raw_value(), stack.raw_value(), userspace)
    , m_address_space(di::move(address_space))
    , m_task_namespace(di::move(task_namespace))
    , m_file_table(di::move(file_table))
    , m_id(id) {}

Task::~Task() {
    m_task_namespace->unregister_task(*this);
}

Expected<di::Arc<Task>> create_kernel_task(TaskNamespace& task_namespace, void (*entry)()) {
    auto entry_address = mm::VirtualAddress(di::to_uintptr(entry));

    auto& address_space = global_state().kernel_address_space;
    auto stack = TRY(address_space.allocate_region(0x2000, mm::RegionFlags::Readable | mm::RegionFlags::Writable));

    auto task_id = TRY(task_namespace.allocate_task_id());
    auto result = TRY(di::try_make_arc<Task>(entry_address, stack + 0x2000, false, address_space.arc_from_this(),
                                             task_namespace.arc_from_this(), task_id, FileTable {}));
    TRY(task_namespace.register_task(*result));
    return result;
}

Expected<di::Arc<Task>> create_user_task(TaskNamespace& task_namespace, FileTable file_table) {
    auto new_address_space = TRY(mm::create_empty_user_address_space());

    auto user_stack = TRY(new_address_space->allocate_region(
        0x10000, mm::RegionFlags::Writable | mm::RegionFlags::User | mm::RegionFlags::Readable));
    auto task_id = TRY(task_namespace.allocate_task_id());
    auto result =
        TRY(di::try_make_arc<Task>(mm::VirtualAddress(0), user_stack + 0x10000, true, di::move(new_address_space),
                                   task_namespace.arc_from_this(), task_id, di::move(file_table)));
    TRY(task_namespace.register_task(*result));
    return result;
}

Expected<void> load_executable(Task& task, di::PathView path) {
    auto raw_data = TRY(lookup_in_initrd(path));

    auto* elf_header = raw_data.typed_pointer_unchecked<elf64::ElfHeader>(0);
    ASSERT_EQ(sizeof(elf64::ProgramHeader), elf_header->program_entry_size);

    asm volatile("cli");
    auto& current_address_space = global_state().scheduler.current_address_space();
    auto& address_space = task.address_space();
    address_space.load();

    auto program_headers = raw_data.typed_span_unchecked<elf64::ProgramHeader>(elf_header->program_table_off,
                                                                               elf_header->program_entry_count);
    // FIXME: handle different program header types and memory protection.
    for (auto& program_header : program_headers) {
        // PT_LOAD
        if (program_header.type != 1) {
            continue;
        }

        auto aligned_size = di::align_up(program_header.memory_size, 4096);
        (void) address_space.allocate_region_at(mm::VirtualAddress(program_header.virtual_addr), aligned_size,
                                                mm::RegionFlags::User | mm::RegionFlags::Readable |
                                                    mm::RegionFlags::Executable | mm::RegionFlags::Writable);

        auto data = di::Span { reinterpret_cast<di::Byte*>(program_header.virtual_addr), aligned_size };

        with_userspace_access([&] {
            di::copy(*raw_data.subspan(program_header.offset, program_header.memory_size), data.data());
        });
    }
    current_address_space.load();

    task.set_instruction_pointer(mm::VirtualAddress(elf_header->entry));
    return {};
}
}
