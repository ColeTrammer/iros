#pragma once

#include <di/util/concepts/const.h>
#include <di/util/concepts/reference.h>

namespace di::util::concepts {
template<typename T>
concept LanguageFunction = (!Const<T const> && !Reference<T>);
}
