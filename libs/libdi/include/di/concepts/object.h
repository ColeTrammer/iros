#pragma once

#include <di/concepts/class.h>
#include <di/concepts/language_array.h>
#include <di/concepts/scalar.h>
#include <di/concepts/union.h>

namespace di::concepts {
template<typename T>
concept Object = Scalar<T> || LanguageArray<T> || Union<T> || Class<T>;
}
