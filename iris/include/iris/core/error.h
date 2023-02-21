#pragma once

#include <di/vocab/expected/prelude.h>
#include <iris/uapi/error.h>

namespace iris {
template<typename T>
using Expected = di::Expected<T, Error>;
}
