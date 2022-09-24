#include <di/prelude.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/core/log.h>
#include <limine.h>

[[noreturn]] static void done() {
    for (;;) {
        asm volatile("mov $52, %eax\n"
                     "cli\n"
                     "hlt\n");
    }
    di::unreachable();
}

static void handler() {
    iris::debug_log("Hello, World - from segfault\n"_sv);
    done();
}

struct [[gnu::packed]] IDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_idt(IDTR descriptor) {
    asm("lidtq %0" : : "m"(descriptor));
}

static auto idt = di::Array<iris::x86::amd64::idt::Entry, 256> {};

extern "C" {
void iris_main() {
    iris::debug_log("Hello, World\n"_sv);

    using namespace iris::x86::amd64::idt;

    auto handler_address = reinterpret_cast<u64>(&handler);
    auto pf_entry = Entry(Present(true), Type(Type::InterruptGate), SegmentSelector(0x28), TargetLow(handler_address & 0xFFFF),
                          TargetMid((handler_address >> 16) & 0xFFFF), TargetHigh(handler_address >> 32));
    idt[14] = pf_entry;

    auto idtr = IDTR { sizeof(idt) - 1, reinterpret_cast<u64>(idt.data()) };
    load_idt(idtr);

    iris::debug_log("Hello, World - again\n"_sv);

    asm volatile("mov $0x1234, %rax\n"
                 "xor %rax, %rax\n"
                 "mov (%rax), %rax\n");

    iris::debug_log("Hello, World - again...\n"_sv);

    done();
}
}