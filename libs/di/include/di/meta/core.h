#pragma once

namespace di::meta {
template<typename... Types>
struct List;

template<auto... vs>
struct ListV {};
}

namespace di::concepts {
template<typename T>
concept Trait = requires { typename T::Type; };
}

namespace di::meta {
template<typename T>
struct TypeConstant {
    using Type = T;

    template<typename...>
    using Invoke = Type;
};

template<concepts::Trait T>
using Type = T::Type;

/// @brief This is a helper template to prevent C++ from deducing the type of template argument.
///
/// This will force users/callers to specify the template types inside the <> brackets.
template<typename T>
using TypeIdentity = Type<TypeConstant<T>>;

namespace detail {
    template<typename T>
    struct RemoveConstHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveConstHelper<T const> : TypeConstant<T> {};
}

template<typename T>
using RemoveConst = Type<detail::RemoveConstHelper<T>>;

namespace detail {
    template<typename T>
    struct RemoveVolatile : TypeConstant<T> {};

    template<typename T>
    struct RemoveVolatile<T volatile> : TypeConstant<T> {};
}

template<typename T>
using RemoveVolatile = Type<detail::RemoveVolatile<T>>;

template<typename T>
using RemoveCV = RemoveConst<RemoveVolatile<T>>;

namespace detail {
    template<typename T>
    struct RemoveReferenceHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveReferenceHelper<T&> : TypeConstant<T> {};

    template<typename T>
    struct RemoveReferenceHelper<T&&> : TypeConstant<T> {};
}

template<typename T>
using RemoveReference = Type<detail::RemoveReferenceHelper<T>>;

template<typename T>
using RemoveCVRef = RemoveCV<RemoveReference<T>>;

template<auto val, typename T = meta::RemoveCVRef<decltype(val)>>
struct Constexpr;

namespace detail {
    template<bool value, typename T, typename U>
    struct ConditionalHelper : TypeConstant<T> {};

    template<typename T, typename U>
    struct ConditionalHelper<false, T, U> : TypeConstant<U> {};
}

template<bool value, typename T, typename U>
using Conditional = detail::ConditionalHelper<value, T, U>::Type;
}

namespace di::concepts {
/// @brief This concept is used with static_assert() to cause the static assert
/// to fail only when the template has been instantiated.
///
/// This is useful
/// for failing compilation from within an if constexpxr block.
template<typename...>
concept AlwaysFalse = false;

/// @brief This concept is used with static_assert() to stop compilation
/// if any provided type is not well-formed.
template<typename...>
concept AlwaysTrue = true;

namespace detail {
    template<typename T, typename U>
    constexpr inline auto same_as_helper = false;

    template<typename T>
    constexpr inline auto same_as_helper<T, T> = true;
}

template<typename T, typename U>
concept SameAs = detail::same_as_helper<T, U>;

template<typename T, typename... Types>
concept OneOf = (SameAs<T, Types> || ...);

namespace detail {
    template<typename T>
    constexpr inline bool language_void_helper = false;

    template<>
    constexpr inline bool language_void_helper<void> = true;
}

template<typename T>
concept LanguageVoid = detail::language_void_helper<meta::RemoveCV<T>>;

namespace detail {
    template<typename T, template<typename...> typename Template>
    constexpr inline bool instance_of_helper = false;

    template<typename... Types, template<typename...> typename Template>
    constexpr inline bool instance_of_helper<Template<Types...>, Template> = true;
}

template<typename T, template<typename...> typename Template>
concept InstanceOf = detail::instance_of_helper<T, Template>;

namespace detail {
    template<typename T, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper = false;

    template<auto... values, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper<Template<values...>, Template> = true;
}

template<typename T, template<auto...> typename Template>
concept InstanceOfV = detail::instance_of_v_helper<T, Template>;

namespace detail {
    template<typename T, template<template<typename...> typename...> typename Template>
    constexpr inline bool instance_of_template_helper = false;

    template<template<typename...> typename... Templates, template<template<typename...> typename...> typename Template>
    constexpr inline bool instance_of_template_helper<Template<Templates...>, Template> = true;
}

template<typename T, template<template<typename...> typename...> typename Template>
concept InstanceOfT = detail::instance_of_template_helper<T, Template>;

template<typename T>
concept TypeList = InstanceOf<T, meta::List>;
}

namespace di {
using concepts::SameAs;
}
