#pragma once

#include <di/meta/operations.h>
#include <di/meta/vocab.h>

namespace di::vocab {
template<typename T, typename U>
concept ConstructibleFromCRefOptional =
    concepts::Optional<U> &&
    (concepts::ConstructibleFrom<T, U&> || concepts::ConstructibleFrom<T, U const&> ||
     concepts::ConstructibleFrom<T, U&&> || concepts::ConstructibleFrom<T, U const&&> ||
     concepts::ImplicitlyConvertibleTo<U&, T> || concepts::ImplicitlyConvertibleTo<U const&, T> ||
     concepts::ImplicitlyConvertibleTo<U&&, T> || concepts::ImplicitlyConvertibleTo<U const&&, T>);
}
