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
namespace detail {
    template<typename T, typename U>
    constexpr inline auto same_as_helper = false;

    template<typename T>
    constexpr inline auto same_as_helper<T, T> = true;
}

template<typename T, typename U>
concept SameAs = detail::same_as_helper<T, U>;
}

namespace di::concepts {
namespace details {
    template<typename T, template<typename...> typename Template>
    constexpr inline bool instance_of_helper = false;

    template<typename... Types, template<typename...> typename Template>
    constexpr inline bool instance_of_helper<Template<Types...>, Template> = true;
}

template<typename T, template<typename...> typename Template>
concept InstanceOf = details::instance_of_helper<T, Template>;

namespace details {
    template<typename T, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper = false;

    template<auto... values, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper<Template<values...>, Template> = true;
}

template<typename T, template<auto...> typename Template>
concept InstanceOfV = details::instance_of_v_helper<T, Template>;

template<typename T>
concept TypeList = InstanceOf<T, meta::List>;
}

namespace di {
using concepts::SameAs;
}
