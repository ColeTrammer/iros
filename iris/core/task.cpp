#include <iris/core/error.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/task.h>
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
Expected<di::Box<Task>> create_kernel_task(void (*entry)()) {
    auto entry_address = mm::VirtualAddress(di::to_uintptr(entry));

    auto& address_space = global_state().kernel_address_space;
    auto stack = TRY(address_space.allocate_region(0x2000));

    return di::try_box<Task>(entry_address, stack + 0x2000, false, address_space.arc_from_this());
}

Expected<di::Box<Task>> create_user_task(di::PathView path) {
    auto raw_data = TRY(lookup_in_initrd(path));

    auto* elf_header = raw_data.typed_pointer_unchecked<elf64::ElfHeader>(0);
    println("entry={:x}"_sv, elf_header->entry);
    ASSERT_EQ(sizeof(elf64::ProgramHeader), elf_header->program_entry_size);

    auto new_address_space = TRY(mm::create_empty_user_address_space());

    // FIXME: we need to also disable preemption while accessing this address space.
    new_address_space->load();

    auto program_headers = raw_data.typed_span_unchecked<elf64::ProgramHeader>(elf_header->program_table_off,
                                                                               elf_header->program_entry_count);
    for (auto& program_header : program_headers) {
        println("type={}"_sv, program_header.type);
        println("addr={:x}"_sv, program_header.virtual_addr);
        println("file={:x}"_sv, program_header.file_size);
        println("memory={:x}"_sv, program_header.memory_size);

        // PT_LOAD
        if (program_header.type != 1) {
            continue;
        }

        auto aligned_size = di::align_up(program_header.memory_size, 4096);
        (void) new_address_space->allocate_region_at(iris::mm::VirtualAddress(program_header.virtual_addr),
                                                     aligned_size);

        auto data = di::Span { reinterpret_cast<di::Byte*>(program_header.virtual_addr), aligned_size };
        di::copy(*raw_data.subspan(program_header.offset, program_header.memory_size), data.data());
    }

    auto kernel_stack = TRY(global_state().kernel_address_space.allocate_region(0x2000));
    return di::try_box<Task>(mm::VirtualAddress(elf_header->entry), kernel_stack, true, di::move(new_address_space));
}
}