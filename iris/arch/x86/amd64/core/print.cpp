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
