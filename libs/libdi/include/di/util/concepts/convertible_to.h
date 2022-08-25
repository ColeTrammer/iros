#pragma once

#include <di/util/concepts/explicitly_convertible_to.h>
#include <di/util/concepts/implicitly_convertible_to.h>

namespace di::util::concepts {
template<typename From, typename To>
concept ConvertibleTo = ImplicitlyConvertibleTo<From, To> && ExplicitlyConvertibleTo<From, To>;
}
