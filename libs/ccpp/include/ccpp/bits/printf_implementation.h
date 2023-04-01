#pragma once

#include <dius/prelude.h>
#include <stdarg.h>

namespace ccpp {
di::Expected<int, dius::PosixCode>
printf_implementation(di::FunctionRef<di::Expected<void, dius::PosixCode>(di::TransparentStringView)> write_exactly,
                      char const* format, va_list args);
}
