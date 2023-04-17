#pragma once

#include <di/container/string/prelude.h>
#include <di/function/container/prelude.h>
#include <dius/error.h>
#include <stdarg.h>

namespace ccpp {
di::Expected<int, dius::PosixCode>
printf_implementation(di::FunctionRef<di::Expected<void, dius::PosixCode>(di::TransparentStringView)> write_exactly,
                      char const* format, va_list args);
}
