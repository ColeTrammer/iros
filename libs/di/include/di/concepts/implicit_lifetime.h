#pragma once

#include <di/concepts/aggregate.h>
#include <di/concepts/language_array.h>
#include <di/concepts/scalar.h>
#include <di/concepts/trivially_copy_constructible.h>
#include <di/concepts/trivially_default_constructible.h>
#include <di/concepts/trivially_destructible.h>
#include <di/concepts/trivially_move_constructible.h>

namespace di::concepts {
// An implicit life time type is one for which the object's lifetime can be started without an explicit call to its
// constructor. This effectively means the type is trivially destructible and trivially constructible somehow. This
// can be used to constrain operations which take a pointers to raw memory and cast it to a proper C++ type.
// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2674r1.pdf. In the future, this should call to a
// compiler built-in.
template<typename T>
concept ImplicitLifetime = (Scalar<T> || LanguageArray<T> || Aggregate<T> ||
                            (TriviallyDestructible<T> &&
                             (TriviallyDefaultConstructible<T> || TriviallyCopyConstructible<T> ||
                              TriviallyMoveConstructible<T>) ));
}
