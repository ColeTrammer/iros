#include <di/prelude.h>
#include <dius/prelude.h>

#ifdef __linux__
#include <asm/prctl.h>
#endif

extern "C" {
extern void (*__preinit_array_start[])(int, char**, char**);
extern void (*__preinit_array_end[])(int, char**, char**);
extern void (*__init_array_start[])(int, char**, char**);
extern void (*__init_array_end[])(int, char**, char**);

extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
}

extern "C" int main(int, char**, char**);

extern "C" [[noreturn]] [[gnu::naked]] void _start() {
#ifdef __linux__
    asm volatile("xor %rbp, %rbp\n"
                 "mov (%rsp), %edi\n"
                 "lea 8(%rsp), %rsi\n"
                 "lea 16(%rsp ,%rdi ,8), %rdx\n"
                 "call dius_entry\n");
#elif defined(DIUS_PLATFORM_IROS)
    asm volatile("call dius_entry\n");
#endif
}

extern "C" di::exec::ElfHeader<> __ehdr_start;

struct ThreadControlBlock {
    ThreadControlBlock* self;
};

extern "C" [[noreturn]] void dius_entry(int argc, char** argv, char** envp) {
    auto* elf_header = di::addressof(__ehdr_start);

    // FIXME: also consider the program header size.
    auto program_header_offset = elf_header->program_table_off;
    auto program_header_count = elf_header->program_entry_count;

    // NOTE: we don't need to validate the executable since the kernel already did, and the worst we can do is crash.
    auto program_headers = di::Span { reinterpret_cast<di::exec::ElfProgramHeader<> const*>(
                                          reinterpret_cast<di::Byte const*>(elf_header) + program_header_offset),
                                      program_header_count };

    auto [tls_address, tls_data_size, tls_size, tls_alignment] = [&] -> di::Tuple<uptr, usize, usize, usize> {
        auto tls_segment = di::find_if(program_headers, [](auto const& header) {
            return header.type == di::exec::ElfProgramHeaderType::Tls;
        });

        if (tls_segment == program_headers.end()) {
            return di::make_tuple(0, 0, 0, 16);
        }
        auto address = tls_segment->virtual_addr;
        auto data_size = tls_segment->file_size;
        auto size = tls_segment->memory_size;
        auto alignment = tls_segment->align;
        return di::make_tuple(address, data_size, size, alignment);
    }();

    // Setup TLS.
    auto alignment = di::max(tls_alignment, alignof(ThreadControlBlock));
    auto size = di::align_up(tls_size, alignment) + sizeof(ThreadControlBlock);
    auto* storage = reinterpret_cast<di::Byte*>(::operator new(size, std::align_val_t { alignment }, std::nothrow));
    ASSERT(storage);

    auto* thread_control_block = reinterpret_cast<ThreadControlBlock*>(storage + di::align_up(tls_size, alignment));
    auto* tls = reinterpret_cast<di::Byte*>(thread_control_block) - tls_size;

    thread_control_block->self = thread_control_block;
    di::copy_n(reinterpret_cast<di::Byte const*>(tls_address), tls_data_size, tls);
    di::fill_n(tls + tls_data_size, tls_size - tls_data_size, 0_b);

#ifdef DIUS_PLATFORM_LINUX
    (void) dius::system::system_call<i32>(dius::system::Number::arch_prctl, ARCH_SET_FS, thread_control_block);
#elif defined(DIUS_PLATFORM_IROS)
    (void) dius::system::system_call<i32>(dius::system::Number::set_userspace_thread_pointer, thread_control_block);
#endif

    iptr preinit_size = __preinit_array_end - __preinit_array_start;
    for (iptr i = 0; i < preinit_size; i++) {
        (*__preinit_array_start[i])(argc, argv, envp);
    }

    iptr init_size = __init_array_end - __init_array_start;
    for (iptr i = 0; i < init_size; i++) {
        (*__init_array_start[i])(argc, argv, envp);
    }

    dius::system::exit_process(__extension__ main(argc, argv, envp));
}
