#pragma once

#include <di/concepts/explicitly_convertible_to.h>
#include <di/concepts/implicitly_convertible_to.h>

namespace di::concepts {
template<typename From, typename To>
concept ConvertibleTo = ImplicitlyConvertibleTo<From, To> && ExplicitlyConvertibleTo<From, To>;
}
