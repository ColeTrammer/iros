#pragma once

#include <stdarg.h>

#include "di/container/string/prelude.h"
#include "di/function/container/prelude.h"
#include "dius/error.h"

namespace ccpp {
di::Expected<int, di::GenericCode>
printf_implementation(di::FunctionRef<di::Expected<void, di::GenericCode>(di::TransparentStringView)> write_exactly,
                      char const* format, va_list args);
}
