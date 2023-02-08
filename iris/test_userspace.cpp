extern "C" [[gnu::naked]] [[noreturn]] void _start() {
    for (;;) {
        asm volatile("int $0x80");
    }
}