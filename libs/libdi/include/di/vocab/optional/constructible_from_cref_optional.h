#pragma once

#include <di/util/concepts/constructible_from.h>
#include <di/util/concepts/implicitly_convertible_to.h>
#include <di/util/concepts/optional.h>

namespace di::vocab::optional {
template<typename T, typename U>
concept ConstructibleFromCRefOptional = util::concepts::Optional<U> &&
                                        (util::concepts::ConstructibleFrom<T, U&> || util::concepts::ConstructibleFrom<T, U const&> ||
                                         util::concepts::ConstructibleFrom<T, U &&> || util::concepts::ConstructibleFrom<T, U const &&> ||
                                         util::concepts::ImplicitlyConvertibleTo<U&, T> ||
                                         util::concepts::ImplicitlyConvertibleTo<U const&, T> ||
                                         util::concepts::ImplicitlyConvertibleTo<U &&, T> ||
                                         util::concepts::ImplicitlyConvertibleTo<U const &&, T>);
}
