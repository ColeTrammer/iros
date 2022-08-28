#pragma once

#include <di/concepts/base_of.h>
#include <di/concepts/implicitly_convertible_to.h>

namespace di::concepts {
template<typename Derived, typename Base>
concept DerivedFrom = BaseOf<Base, Derived> && ImplicitlyConvertibleTo<Derived const volatile*, Base const volatile*>;
}
