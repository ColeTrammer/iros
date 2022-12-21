#pragma once

#include <di/vocab/error/error.h>
#include <di/vocab/expected/prelude.h>

namespace di::vocab {
template<typename T>
using Result = Expected<T, Error>;
}