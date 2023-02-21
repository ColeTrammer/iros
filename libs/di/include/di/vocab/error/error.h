#pragma once

#include <di/vocab/error/erased_status_code.h>

namespace di::vocab {
using Error = StatusCode<Erased<long>>;
}
