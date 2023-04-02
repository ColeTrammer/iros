#include <di/prelude.h>
#include <dius/prelude.h>

#ifdef __linux__
#include <asm/prctl.h>
#endif

namespace dius::runtime {
extern "C" {
extern void (*__preinit_array_start[])(int, char**, char**);
extern void (*__preinit_array_end[])(int, char**, char**);
extern void (*__init_array_start[])(int, char**, char**);
extern void (*__init_array_end[])(int, char**, char**);

extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
}

extern "C" int main(int, char**, char**);

extern "C" di::exec::ElfHeader<> __ehdr_start;

static constinit dius::runtime::TlsInfo s_tls_info {};

dius::runtime::TlsInfo get_tls_info() {
    return s_tls_info;
}

extern "C" [[noreturn]] void dius_entry(int argc, char** argv, char** envp) {
    auto* elf_header = di::addressof(__ehdr_start);

    // FIXME: also consider the program header size.
    auto program_header_offset = elf_header->program_table_off;
    auto program_header_count = elf_header->program_entry_count;

    // NOTE: we don't need to validate the executable since the kernel already did, and the worst we can do is crash.
    auto program_headers = di::Span { reinterpret_cast<di::exec::ElfProgramHeader<> const*>(
                                          reinterpret_cast<byte const*>(elf_header) + program_header_offset),
                                      program_header_count };

    s_tls_info = [&] -> dius::runtime::TlsInfo {
        auto tls_segment = di::find_if(program_headers, [](auto const& header) {
            return header.type == di::exec::ElfProgramHeaderType::Tls;
        });

        if (tls_segment == program_headers.end()) {
            return { di::Span<byte const> {}, 0, 16 };
        }
        auto address = tls_segment->virtual_addr;
        auto data_size = tls_segment->file_size;
        auto size = tls_segment->memory_size;
        auto alignment = tls_segment->align;
        return { di::Span { reinterpret_cast<byte const*>(address.value()), data_size }, size, alignment };
    }();

    // Setup TLS.
    auto thread_control_block = dius::PlatformThread::create(get_tls_info());
    ASSERT(thread_control_block);

#ifdef DIUS_PLATFORM_LINUX
    (void) dius::system::system_call<i32>(dius::system::Number::arch_prctl, ARCH_SET_FS, thread_control_block->get());
#elif defined(DIUS_PLATFORM_IROS)
    (void) dius::system::system_call<i32>(dius::system::Number::set_userspace_thread_pointer, 0,
                                          thread_control_block->get());
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

#ifdef __linux__
extern "C" [[noreturn]] [[gnu::naked]] void _start() {
    asm volatile("xor %rbp, %rbp\n"
                 "mov (%rsp), %edi\n"
                 "lea 8(%rsp), %rsi\n"
                 "lea 16(%rsp ,%rdi ,8), %rdx\n"
                 "call dius_entry\n");
}
#elif __iros__
extern "C" [[noreturn]] void _start(di::TransparentStringView* argv, usize argc, di::TransparentStringView* envp,
                                    usize envc) {
    // NOTE: although the kernel passes the arguments and enviornment as pointer-length pairs, POSIX and the C standard
    //       require a different format. In the future, dius applications will be able to opt-in to consuming kernel's
    //       format directly, but for now, we need to convert it.

    auto** c_argv = reinterpret_cast<char**>(argv);
    auto** c_envp = reinterpret_cast<char**>(envp);

    auto* null_pointer = static_cast<char*>(nullptr);

    // Convert the string pointers into raw u64 arrays to prevent UB, and then reuse the stack area as a null-terminated
    // pointer array. To handle empty arguments or enviornment, we reserve a null-pointer on the stack.

    auto argv_words = di::Span { reinterpret_cast<u64*>(argv), argc * sizeof(di::TransparentStringView) / sizeof(u64) };
    if (argc == 0) {
        c_argv = &null_pointer;
    } else {
        *di::copy(argv_words | di::stride(2), argv_words.begin()).out = 0;
    }

    auto envp_words = di::Span { reinterpret_cast<u64*>(envp), envc * sizeof(di::TransparentStringView) / sizeof(u64) };
    if (envc == 0) {
        c_envp = &null_pointer;
    } else {
        *di::copy(envp_words | di::stride(2), envp_words.begin()).out = 0;
    }

    dius_entry(int(argc), c_argv, c_envp);
}
#endif
}
