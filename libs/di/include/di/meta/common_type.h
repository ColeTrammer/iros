#pragma once

#include <di/concepts/same_as.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/remove_reference.h>
#include <di/meta/type_constant.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T, typename U>
struct CustomCommonType {};

namespace detail {
    template<typename T, typename U>
    concept NeedsDecay = (!concepts::SameAs<T, meta::Decay<T>> || !concepts::SameAs<U, meta::Decay<U>>);

    template<typename T, typename U>
    concept HasCustomCommonType = requires { typename CustomCommonType<T, U>::Type; };

    template<typename T, typename U>
    concept ValueCommonType = requires { false ? util::declval<T>() : util::declval<U>(); };

    template<typename T, typename U>
    concept ReferenceCommonType =
        requires {
            false ? util::declval<meta::RemoveReference<T> const&>() : util::declval<meta::RemoveReference<U> const&>();
        };

    template<typename... Types>
    struct CommonTypeHelper {};

    template<typename T>
    struct CommonTypeHelper<T> : CommonTypeHelper<T, T> {};

    template<typename T, typename U>
    requires(NeedsDecay<T, U>)
    struct CommonTypeHelper<T, U> : CommonTypeHelper<meta::Decay<T>, meta::Decay<U>> {};

    template<typename T, typename U>
    requires(!NeedsDecay<T, U> && HasCustomCommonType<T, U>)
    struct CommonTypeHelper<T, U> : TypeConstant<typename CustomCommonType<T, U>::Type> {};

    template<typename T, typename U>
    requires(!NeedsDecay<T, U> && !HasCustomCommonType<T, U> && ValueCommonType<T, U>)
    struct CommonTypeHelper<T, U>
        : TypeConstant<meta::Decay<decltype(false ? util::declval<T>() : util::declval<U>())>> {};

    template<typename T, typename U>
    requires(!NeedsDecay<T, U> && !HasCustomCommonType<T, U> && !ValueCommonType<T, U> && ReferenceCommonType<T, U>)
    struct CommonTypeHelper<T, U>
        : TypeConstant<meta::Decay<decltype(false ? util::declval<meta::RemoveReference<T> const&>()
                                                  : util::declval<meta::RemoveReference<U> const&>())>> {};

    template<typename T, typename U, typename W, typename... Rest>
    requires(requires { typename CommonTypeHelper<T, U>::Type; })
    struct CommonTypeHelper<T, U, W, Rest...> : CommonTypeHelper<typename CommonTypeHelper<T, U>::Type, W, Rest...> {};
}

template<typename... Types>
requires(requires { typename detail::CommonTypeHelper<Types...>::Type; })
using CommonType = detail::CommonTypeHelper<Types...>::Type;
}
