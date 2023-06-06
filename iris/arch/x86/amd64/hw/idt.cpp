#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/global_state.h>

namespace iris::x86::amd64::idt {
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

                 "mov %0, %%rdi\n"
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

void init_idt() {
    auto& idt = global_state_in_boot().arch_readonly_state.idt;

    di::function::unpack<di::meta::MakeIntegerSequence<int, 256>>([&]<int... indices>(di::meta::ListV<indices...>) {
        auto addresses = di::Array { di::to_uintptr(get_irq_handler<indices>())... };
        for (auto [n, handler_address] : di::enumerate(addresses)) {
            idt[n] = Entry(Present { true }, Type(Type::InterruptGate), SegmentSelector { 5 * 8 },
                           TargetLow(handler_address & 0xFFFF), TargetMid((handler_address >> 16) & 0xFFFF),
                           TargetHigh(handler_address >> 32), DPL { 3 });
        }
    });

    load_idt();
}

void load_idt() {
    auto const& idt = global_state().arch_readonly_state.idt;

    auto idtr = iris::x86::amd64::IDTR { sizeof(idt) - 1, di::to_uintptr(idt.data()) };
    iris::x86::amd64::load_idt(idtr);
}
}
