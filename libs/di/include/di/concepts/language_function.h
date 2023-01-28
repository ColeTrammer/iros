#pragma once

#include <di/concepts/const.h>
#include <di/concepts/reference.h>

namespace di::concepts {
template<typename T>
concept LanguageFunction = (!Const<T const> && !Reference<T>);
}
