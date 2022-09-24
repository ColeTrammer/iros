#pragma once

#include <di/prelude.h>

namespace iris {
enum class Error {
    OutOfMemory,
};

template<typename T>
using Expected = di::Expected<T, Error>;
}