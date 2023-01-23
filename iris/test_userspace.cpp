extern "C" [[gnu::naked]] [[noreturn]] void _start() {
    asm volatile("int $0x80");
}