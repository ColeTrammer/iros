#pragma once

#include <dius/prelude.h>
#include <stdarg.h>

namespace ccpp {
di::Expected<int, dius::PosixCode>
scanf_implementation(di::FunctionRef<di::Expected<di::Optional<char>, dius::PosixCode>()> read_next, char const* format,
                     va_list args);
}
