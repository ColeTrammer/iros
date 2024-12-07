#pragma once

#include "di/vocab/error/error.h"
#include "di/vocab/expected/prelude.h"

namespace di::vocab {
template<typename T = void>
using Result = Expected<T, Error>;
}

namespace di {
using vocab::Result;
}
