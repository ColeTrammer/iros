#pragma once

#include <di/concepts/language_void.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::concepts {
// Implicit conversion for this test refers to the ability to return a value of function
// from a type. In particular, []() -> long { return static_cast<int>(0); }, is valid
// because 'int' is implicitly convertible to 'long'. In addition, one can pass 'int'
// to a function expecting a 'long', with no conversion necessary.

// This is checked first by determining if a function pointer which returns the type 'To'
// is valid (arrays with dimensions, like int[42] cannot be returned from functions).

// Secondly, it is checked that a value of type 'From' can be passed to a function expecting
// a value of type 'To'.
template<typename From, typename To>
concept ImplicitlyConvertibleTo =
    (LanguageVoid<From> && LanguageVoid<To>) || requires(void (*function_accepting_to)(To), From&& from) {
        static_cast<To (*)()>(nullptr);
        { function_accepting_to(util::forward<From>(from)) };
    };
}
