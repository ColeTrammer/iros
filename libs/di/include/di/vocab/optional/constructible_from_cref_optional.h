#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/implicitly_convertible_to.h>
#include <di/concepts/optional.h>

namespace di::vocab {
template<typename T, typename U>
concept ConstructibleFromCRefOptional =
    concepts::Optional<U> &&
    (concepts::ConstructibleFrom<T, U&> || concepts::ConstructibleFrom<T, U const&> ||
     concepts::ConstructibleFrom<T, U&&> || concepts::ConstructibleFrom<T, U const&&> ||
     concepts::ImplicitlyConvertibleTo<U&, T> || concepts::ImplicitlyConvertibleTo<U const&, T> ||
     concepts::ImplicitlyConvertibleTo<U&&, T> || concepts::ImplicitlyConvertibleTo<U const&&, T>);
}
