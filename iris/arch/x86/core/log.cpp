#include <iris/core/print.h>

namespace iris {
static inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void log_output_character(c32 value) {
    for (auto byte : di::encoding::convert_to_code_units(di::container::string::Utf8Encoding {}, value)) {
        outb(0xE9, byte);
    }
}
}

namespace di::assert::detail {
void assert_write(char const* data, size_t size) {
    for (size_t i = 0; i != size; i++) {
        iris::log_output_character(data[i]);
    }
}

void assert_terminate() {
    for (;;) {
        asm volatile("cli\n"
                     "hlt\n");
    }
}
}