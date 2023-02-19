#include <di/prelude.h>
#include <iris/arch/x86/amd64/hw/pic.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

namespace iris::arch {
[[noreturn]] static void done() {
    for (;;) {
        asm volatile("mov $52, %eax\n"
                     "cli\n"
                     "hlt\n");
    }
    di::unreachable();
}

extern "C" void generic_irq_handler(int irq, iris::arch::TaskState* task_state, int error_code) {
    iris::println("got IRQ {}, error_code={}"_sv, irq, error_code);

    if (irq == 32) {
        iris::x86::amd64::send_eoi(0);
        iris::global_state().scheduler.save_state_and_run_next(task_state);
    }

    if (irq == 0x80) {
        // System call.
        auto string_base = task_state->rdi;
        auto string_length = task_state->rsi;
        auto string = di::TransparentStringView { reinterpret_cast<char const*>(string_base), string_length };
        iris::print("{}"_sv, string);

        iris::global_state().scheduler.save_state_and_run_next(task_state);
    }
    done();
}

#define DEFINE_IRQ_HANDLER(n)                               \
    [[gnu::naked]] [[noreturn]] static void handler_##n() { \
        asm volatile("push %rax\n"                          \
                     "push %rbx\n"                          \
                     "push %rcx\n"                          \
                     "push %rdx\n"                          \
                     "push %rsi\n"                          \
                     "push %rdi\n"                          \
                     "push %rbp\n"                          \
                     "push %r8\n"                           \
                     "push %r9\n"                           \
                     "push %r10\n"                          \
                     "push %r11\n"                          \
                     "push %r12\n"                          \
                     "push %r13\n"                          \
                     "push %r14\n"                          \
                     "push %r15\n "                         \
                                                            \
                     "mov $" #n ", %rdi\n"                  \
                     "mov %rsp, %rsi\n"                     \
                     "xor %rdx, %rdx\n"                     \
                     "callq generic_irq_handler\n"          \
                                                            \
                     "pop %r15\n"                           \
                     "pop %r14\n"                           \
                     "pop %r13\n"                           \
                     "pop %r12\n"                           \
                     "pop %r11\n"                           \
                     "pop %r10\n"                           \
                     "pop %r9\n"                            \
                     "pop %r8\n"                            \
                     "pop %rbp\n"                           \
                     "pop %rdi\n"                           \
                     "pop %rsi\n"                           \
                     "pop %rdx\n"                           \
                     "pop %rcx\n"                           \
                     "pop %rbx\n"                           \
                     "pop %rax\n"                           \
                                                            \
                     "iretq\n");                            \
    }

#define DEFINE_IRQ_HANDLER_WITH_ERROR_CODE(n)               \
    [[gnu::naked]] [[noreturn]] static void handler_##n() { \
        asm volatile("xor (%rsp), %rax\n"                   \
                     "xor %rax, (%rsp)\n"                   \
                     "xor (%rsp), %rax\n"                   \
                                                            \
                     "push %rbx\n"                          \
                     "push %rcx\n"                          \
                     "push %rdx\n"                          \
                     "push %rsi\n"                          \
                     "push %rdi\n"                          \
                     "push %rbp\n"                          \
                     "push %r8\n"                           \
                     "push %r9\n"                           \
                     "push %r10\n"                          \
                     "push %r11\n"                          \
                     "push %r12\n"                          \
                     "push %r13\n"                          \
                     "push %r14\n"                          \
                     "push %r15\n "                         \
                                                            \
                     "mov $" #n ", %rdi\n"                  \
                     "mov %rsp, %rsi\n"                     \
                     "mov %rax, %rdx\n"                     \
                     "callq generic_irq_handler\n"          \
                                                            \
                     "pop %r15\n"                           \
                     "pop %r14\n"                           \
                     "pop %r13\n"                           \
                     "pop %r12\n"                           \
                     "pop %r11\n"                           \
                     "pop %r10\n"                           \
                     "pop %r9\n"                            \
                     "pop %r8\n"                            \
                     "pop %rbp\n"                           \
                     "pop %rdi\n"                           \
                     "pop %rsi\n"                           \
                     "pop %rdx\n"                           \
                     "pop %rcx\n"                           \
                     "pop %rbx\n"                           \
                     "pop %rax\n"                           \
                                                            \
                     "iretq\n");                            \
    }

#define FOR_EACH_INTEGER_LESS_THEN_256(s)                                                                              \
    s(0) s(1) s(2) s(3) s(4) s(5) s(6) s(7) s(8) s(9) s(10) s(11) s(12) s(13) s(14) s(15) s(16) s(17) s(18) s(19)      \
        s(20) s(21) s(22) s(23) s(24) s(25) s(26) s(27) s(28) s(29) s(30) s(31) s(32) s(33) s(34) s(35) s(36) s(37)    \
            s(38) s(39) s(40) s(41) s(42) s(43) s(44) s(45) s(46) s(47) s(48) s(49) s(50) s(51) s(52) s(53) s(54)      \
                s(55) s(56) s(57) s(58) s(59) s(60) s(61) s(62) s(63) s(64) s(65) s(66) s(67) s(68) s(69) s(70) s(71)  \
                    s(72) s(73) s(74) s(75) s(76) s(77) s(78) s(79) s(80) s(81) s(82) s(83) s(84) s(85) s(86) s(87) s( \
                        88) s(89) s(90) s(91) s(92) s(93) s(94) s(95) s(96) s(97) s(98) s(99) s(100) s(101) s(102)     \
                        s(103) s(104) s(105) s(106) s(107) s(108) s(109) s(110) s(111) s(112) s(113) s(114) s(115) s(  \
                            116) s(117) s(118) s(119) s(120) s(121) s(122) s(123) s(124) s(125) s(126) s(127) s(128)   \
                            s(129) s(130) s(131) s(132) s(133) s(134) s(135) s(136) s(137) s(138) s(139) s(140) s(141) \
                                s(142) s(143) s(144) s(145) s(146) s(147) s(148) s(149) s(150) s(151) s(152) s(153)    \
                                    s(154) s(155) s(156) s(157) s(158) s(159) s(160) s(161) s(162) s(163) s(164)       \
                                        s(165) s(166) s(167) s(168) s(169) s(170) s(171) s(172) s(173) s(174) s(175)   \
                                            s(176) s(177) s(178) s(179) s(180) s(181) s(182) s(183) s(184) s(185)      \
                                                s(186) s(187) s(188) s(189) s(190) s(191) s(192) s(193) s(194) s(195)  \
                                                    s(196) s(197) s(198) s(199) s(200) s(201) s(202) s(203) s(204)     \
                                                        s(205) s(206) s(207) s(208) s(209) s(210) s(211) s(212) s(213) \
                                                            s(214) s(215) s(216) s(217) s(218) s(219) s(220) s(221)    \
                                                                s(222) s(223) s(224) s(225) s(226) s(227) s(228)       \
                                                                    s(229) s(230) s(231) s(232) s(233) s(234) s(235)   \
                                                                        s(236) s(237) s(238) s(239) s(240) s(241)      \
                                                                            s(242) s(243) s(244) s(245) s(246) s(247)  \
                                                                                s(248) s(249) s(250) s(251) s(252)     \
                                                                                    s(253) s(254) s(255)

// For a list of x86_64 IRQ with push an error code onto the stack, see the exception table
// on the OSDEV wiki: https://wiki.osdev.org/Exceptions
#define HAS_ERROR_CODE(n)   DI_IS_PROBE(DI_PRIMITIVE_CAT(__HAS_ERROR_CODE_, n))
#define __HAS_ERROR_CODE_8  DI_PROBE()
#define __HAS_ERROR_CODE_10 DI_PROBE()
#define __HAS_ERROR_CODE_11 DI_PROBE()
#define __HAS_ERROR_CODE_12 DI_PROBE()
#define __HAS_ERROR_CODE_13 DI_PROBE()
#define __HAS_ERROR_CODE_14 DI_PROBE()
#define __HAS_ERROR_CODE_17 DI_PROBE()
#define __HAS_ERROR_CODE_21 DI_PROBE()
#define __HAS_ERROR_CODE_28 DI_PROBE()
#define __HAS_ERROR_CODE_29 DI_PROBE()
#define __HAS_ERROR_CODE_30 DI_PROBE()

#define DEFINE_PROPER_IRQ_HANDLER(n) \
    DI_ID(DI_IF(HAS_ERROR_CODE(n))(DEFINE_IRQ_HANDLER_WITH_ERROR_CODE)(DEFINE_IRQ_HANDLER))(n)

FOR_EACH_INTEGER_LESS_THEN_256(DEFINE_PROPER_IRQ_HANDLER)

static char temp_stack[4 * 4096] alignas(4096);

static auto idt = di::Array<iris::x86::amd64::idt::Entry, 256> {};
static auto gdt = di::Array<iris::x86::amd64::sd::SegmentDescriptor, 11> {};
static auto tss = iris::x86::amd64::TSS {};

extern "C" void bsp_cpu_init() {
    iris::arch::cxx_init();

    iris::println(u8"Hello, World"_sv);

    {
        using namespace iris::x86::amd64::idt;

#define IDT_ENTRY(n)                                                                                     \
    {                                                                                                    \
        auto handler_address = di::to_uintptr(&handler_##n);                                             \
        idt[n] = Entry(Present(true), Type(Type::InterruptGate), SegmentSelector(5 * 8),                 \
                       TargetLow(handler_address & 0xFFFF), TargetMid((handler_address >> 16) & 0xFFFF), \
                       TargetHigh(handler_address >> 32), DPL(3));                                       \
    }

        FOR_EACH_INTEGER_LESS_THEN_256(IDT_ENTRY)

        auto idtr = iris::x86::amd64::IDTR { sizeof(idt) - 1, di::to_uintptr(idt.data()) };
        iris::x86::amd64::load_idt(idtr);
    }

    {
        using namespace iris::x86::amd64::ssd;

        // Setup TSS.
        tss.io_map_base = sizeof(tss);
        tss.rsp[0] = di::to_uintptr(temp_stack);
        tss.rsp[1] = di::to_uintptr(temp_stack);
        tss.rsp[2] = di::to_uintptr(temp_stack);
        tss.ist[0] = di::to_uintptr(temp_stack);

        // TSS Descriptor Setup.
        auto tss_descriptor = reinterpret_cast<SystemSegmentDescriptor*>(&gdt[9]);
        auto tss_address = di::to_uintptr(&tss);
        *tss_descriptor = SystemSegmentDescriptor(
            LimitLow(sizeof(tss)), BaseLow(tss_address & 0xFFFF), BaseMidLow((tss_address >> 16) & 0xFF),
            Type(Type::TSS), Present(true), BaseMidHigh((tss_address >> 24) & 0xFF), BaseHigh((tss_address >> 32)));
    }

    {
        using namespace iris::x86::amd64::sd;

        // The layout of the GDT matches the limine boot protocol, although this is not strictly necessary.
        // The 16 bit and 32 bit segments are included to ease future attempts to boot APs.
        // This is the null segment descriptor.
        gdt[0] = SegmentDescriptor();

        // 16 bit Code Descriptor.
        gdt[1] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 16 bit Data Descriptor.
        gdt[2] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 32 bit Code Descriptor.
        gdt[3] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 32 bit Data Descriptor.
        gdt[4] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 64 bit Code Descriptor.
        gdt[5] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit Data Descriptor.
        gdt[6] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 64 bit User Code Descriptor.
        gdt[7] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), DPL(3),
                                   Present(true), LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit User Data Descriptor.
        gdt[8] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), DPL(3), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        auto gdtr = iris::x86::amd64::GDTR { sizeof(gdt) - 1, di::to_uintptr(gdt.data()) };
        iris::x86::amd64::load_gdt(gdtr);

        // Load TSS.
        iris::x86::amd64::load_tr(9 * 8);

        // Load the data segments with NULL segment selector.
        asm volatile("mov %0, %%dx\n"
                     "mov %%dx, %%ds\n"
                     "mov %%dx, %%es\n"
                     "mov %%dx, %%fs\n"
                     "mov %%dx, %%ss\n"
                     "mov %%dx, %%gs\n"
                     :
                     : "i"(0)
                     : "memory", "edx");
    }

    iris::println("Waiting..."_sv);
    iris::x86::amd64::init_pic();

    iris_main();
}

extern "C" [[gnu::naked]] void iris_entry() {
    asm volatile("mov %0, %%rsp\n"
                 "push $0\n"
                 "call bsp_cpu_init\n"
                 :
                 : "r"(temp_stack + sizeof(temp_stack))
                 : "memory");
}
}