static char const message[] = "Hello, World\n";

extern "C" [[gnu::naked]] [[noreturn]] void _start() {
    for (unsigned int i = 0; i < 2; i++) {
        asm volatile("mov %0, %%rdi\n"
                     "mov %1, %%rsi\n"
                     "int $0x80\n" ::"r"(message),
                     "r"(sizeof(message)));
    }
    asm volatile("xor %rax, %rax\n"
                 "mov (%rax), %rax\n");
}