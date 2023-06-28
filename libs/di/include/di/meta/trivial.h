#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/types/in_place_type.h>

namespace di::concepts {
template<typename T>
concept Trivial = __is_trivial(T);

template<typename T, typename U>
concept TriviallyAssignableFrom = __is_trivially_assignable(T, U);

template<typename T, typename... Args>
concept TriviallyConstructibleFrom = __is_trivially_constructible(T, Args...);

template<typename T>
concept TriviallyCopyAssignable = TriviallyAssignableFrom<T&, T const&>;

template<typename T>
concept TriviallyCopyConstructible = TriviallyConstructibleFrom<T, T const&>;

template<typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);

template<typename T>
concept TriviallyDefaultConstructible = TriviallyConstructibleFrom<T>;

template<typename T>
concept TriviallyMoveAssignable = TriviallyAssignableFrom<T&, T&&>;

template<typename T>
concept TriviallyMoveConstructible = TriviallyConstructibleFrom<T, T&&>;

#ifdef DI_CLANG
template<typename T>
concept TriviallyDestructible = __is_trivially_destructible(T);
#else
template<typename T>
concept TriviallyDestructible = Destructible<T> && __has_trivial_destructor(T);
#endif

namespace detail {
    struct TriviallyRelocatableFunction {
        template<typename T>
        constexpr bool operator()(InPlaceType<T>) const {
            if constexpr (TagInvocableTo<TriviallyRelocatableFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return concepts::TriviallyCopyable<T>;
            }
        }
    };
}

constexpr inline auto trivially_relocatable = detail::TriviallyRelocatableFunction {};

template<typename T>
concept TriviallyRelocatable = trivially_relocatable(in_place_type<T>);

/// @brief An implicit life time type is one for which the object's lifetime can be started without an explicit call to
/// its constructor.
///
/// This effectively means the type is trivially destructible and trivially constructible somehow. This
/// can be used to constrain operations which take a pointers to raw memory and cast it to a proper C++ type.
/// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2674r1.pdf. In the future, this should call to a
/// compiler built-in.
template<typename T>
concept ImplicitLifetime =
    (Scalar<T> || LanguageArray<T> || Aggregate<T> ||
     (TriviallyDestructible<T> &&
      (TriviallyDefaultConstructible<T> || TriviallyCopyConstructible<T> || TriviallyMoveConstructible<T>) ));
}
