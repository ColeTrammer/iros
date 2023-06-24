#pragma once

#include <di/meta/compare.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename T, typename... Args>
concept ConstructibleFrom = (!LanguageVoid<T>) &&requires(Args&&... args) { T(util::forward<Args>(args)...); };

template<typename T, typename U>
concept WeaklyAssignableFrom = requires(T lhs, U&& value) {
    { lhs = util::forward<U>(value) } -> SameAs<T>;
};

template<typename T, typename U>
concept AssignableFrom = LValueReference<T> && requires(T lvalue, U&& value) {
    { lvalue = util::forward<U>(value) } -> SameAs<T>;
};

template<typename T>
concept DefaultConstructible = ConstructibleFrom<T>;

template<typename T>
concept DefaultInitializable = requires {
    T();
    T {};
    (void) ::new T;
};

template<typename T>
concept CopyConstructible = ConstructibleFrom<T, T const&>;

template<typename T>
concept CopyAssignable = AssignableFrom<T&, T const&>;

template<typename T>
concept Copyable = CopyConstructible<T> && CopyAssignable<T>;

template<typename T>
concept MoveConstructible = ConstructibleFrom<T, T&&>;

template<typename T>
concept MoveAssignable = AssignableFrom<T&, T>;

template<typename T>
concept Movable = MoveConstructible<T> && MoveAssignable<T>;

namespace detail {
    template<typename T>
    concept DestructibleHelper = requires { util::declval<T&>().~T(); };
}

template<typename T>
concept Destructible = Reference<T> || (!LanguageVoid<T> && !LanguageFunction<T> && !UnboundedLanguageArray<T> &&
                                        detail::DestructibleHelper<meta::RemoveAllExtents<T>>);

namespace detail {
    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to = SameAs<From, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To const> = SameAs<meta::RemoveConst<From>, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To volatile> = SameAs<meta::RemoveVolatile<From>, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To const volatile> = SameAs<meta::RemoveCV<From>, To>;
}

template<typename From, typename To>
concept QualificationConvertibleTo = detail::qualification_convertible_to<From, To>;

/// @brief Implicit conversion for this test refers to the ability to return a value of function from a type.
///
/// In particular, []() -> long { return static_cast<int>(0); }, is valid
/// because 'int' is implicitly convertible to 'long'. In addition, one can pass 'int'
/// to a function expecting a 'long', with no conversion necessary.
///
/// This is checked first by determining if a function pointer which returns the type 'To'
/// is valid (arrays with dimensions, like int[42] cannot be returned from functions).
///
/// Secondly, it is checked that a value of type 'From' can be passed to a function expecting
/// a value of type 'To'.
template<typename From, typename To>
concept ImplicitlyConvertibleTo =
    (LanguageVoid<From> && LanguageVoid<To>) || requires(void (*function_accepting_to)(To), From&& from) {
        static_cast<To (*)()>(nullptr);
        { function_accepting_to(util::forward<From>(from)) };
    };

template<typename From, typename To>
concept ExplicitlyConvertibleTo = requires { static_cast<To>(util::declval<From>()); };

template<typename From, typename To>
concept ConvertibleTo = ImplicitlyConvertibleTo<From, To> && ExplicitlyConvertibleTo<From, To>;

/// @brief This concept requires that the conversion from From to To would not
/// result in converting a derived type to a base type.
///
/// This is useful
/// to prevent slicing when treating pointers as iterators, since an
/// Cat[] can not be viewed the same as an Animal[].
template<typename From, typename To>
concept ConvertibleToNonSlicing =
    ConvertibleTo<From, To> &&
    (!Pointer<meta::Decay<From>> || !Pointer<meta::Decay<To>> ||
     ImplicitlyConvertibleTo<meta::RemovePointer<From> (*)[], meta::RemovePointer<To> (*)[]>);

template<typename Derived, typename Base>
concept DerivedFrom = BaseOf<Base, Derived> && ImplicitlyConvertibleTo<Derived const volatile*, Base const volatile*>;

template<typename T>
concept Semiregular = Copyable<T> && DefaultInitializable<T>;

template<typename T>
concept Regular = Semiregular<T> && EqualityComparable<T>;

namespace detail {
    template<typename T>
    concept BooleanTestableImpl = ConvertibleTo<T, bool>;
}

template<typename T>
concept BooleanTestable = detail::BooleanTestableImpl<T> && requires(T&& value) {
    { !util::forward<T>(value) } -> detail::BooleanTestableImpl;
};

template<typename T>
concept CanReference = requires { typename meta::TypeConstant<T&>; };

template<typename T>
concept Dereferenceable = requires(T& it) {
    { *it } -> CanReference;
};
}
