#pragma once

#include <di/function/container/prelude.h>
#include <dius/error.h>
#include <stdarg.h>

namespace ccpp {
di::Expected<int, di::GenericCode>
scanf_implementation(di::FunctionRef<di::Expected<di::Optional<char>, di::GenericCode>()> read_next, char const* format,
                     va_list args);
}
