#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/core/print.h>
#include <iris/hw/power.h>

namespace iris {
void log_output_character(c32 value) {
    for (auto byte : di::encoding::convert_to_code_units(di::container::string::Utf8Encoding {}, value)) {
        log_output_byte(di::Byte(byte));
    }
}
}

namespace di::assert::detail {
void assert_write(char const* data, size_t size) {
    iris::raw_disable_interrupts();
    for (size_t i = 0; i != size; i++) {
        iris::log_output_character(data[i]);
    }
}

void assert_terminate() {
    iris::println("Assertion failed, shutting down..."_sv);
    iris::hard_shutdown(iris::ShutdownStatus::Error);
}
}
