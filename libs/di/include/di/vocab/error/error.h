#pragma once

#include "di/vocab/error/erased_status_code.h"

namespace di::vocab {
/// The error module is nearly an implementation of P1028R4.
/// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1028r4.pdf
using Error = StatusCode<Erased<long>>;
}

namespace di {
using vocab::Error;
}
