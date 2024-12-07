#pragma once

#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/meta/vocab.h"
#include "di/util/declval.h"
#include "di/util/forward.h"

namespace di::function {
namespace detail {
    template<concepts::MemberFunctionPointer F, typename FirstArg, typename... Args>
    requires(concepts::BaseOf<meta::MemberPointerClass<F>, meta::Decay<FirstArg>>)
    constexpr auto invoke_impl(F f, FirstArg&& first_arg, Args&&... args)
        -> decltype((util::forward<FirstArg>(first_arg).*f)(util::forward<Args>(args)...)) {
        return (util::forward<FirstArg>(first_arg).*f)(util::forward<Args>(args)...);
    }

    template<concepts::MemberFunctionPointer F, typename FirstArg, typename... Args>
    requires(concepts::ReferenceWrapper<meta::Decay<FirstArg>>)
    constexpr auto invoke_impl(F f, FirstArg&& first_arg, Args&&... args)
        -> decltype((first_arg.get().*f)(util::forward<Args>(args)...)) {
        return (first_arg.get().*f)(util::forward<Args>(args)...);
    }

    template<concepts::MemberFunctionPointer F, typename FirstArg, typename... Args>
    constexpr auto invoke_impl(F f, FirstArg&& first_arg, Args&&... args)
        -> decltype(((*util::forward<FirstArg>(first_arg)).*f)(util::forward<Args>(args)...)) {
        return ((*util::forward<FirstArg>(first_arg)).*f)(util::forward<Args>(args)...);
    }

    template<concepts::MemberObjectPointer F, typename Arg>
    requires(concepts::BaseOf<meta::MemberPointerClass<F>, meta::Decay<Arg>>)
    constexpr auto invoke_impl(F f, Arg&& arg) -> decltype(util::forward<Arg>(arg).*f) {
        return util::forward<Arg>(arg).*f;
    }

    template<concepts::MemberObjectPointer F, typename Arg>
    requires(concepts::ReferenceWrapper<meta::Decay<Arg>>)
    constexpr auto invoke_impl(F f, Arg&& arg) -> decltype(arg.get().*f) {
        return arg.get().*f;
    }

    template<concepts::MemberObjectPointer F, typename Arg>
    constexpr auto invoke_impl(F f, Arg&& arg) -> decltype((*util::forward<Arg>(arg)).*f) {
        return (*util::forward<Arg>(arg)).*f;
    }

    template<typename F, typename... Args>
    constexpr auto invoke_impl(F&& f, Args&&... args) -> decltype(util::forward<F>(f)(util::forward<Args>(args)...)) {
        return util::forward<F>(f)(util::forward<Args>(args)...);
    }
}
}

namespace di::concepts {
template<typename... Ts>
concept Invocable = requires(Ts&&... ts) { function::detail::invoke_impl(util::forward<Ts>(ts)...); };
}

namespace di::meta {
template<typename... Ts>
requires(concepts::Invocable<Ts...>)
using InvokeResult = decltype(function::detail::invoke_impl(util::declval<Ts>()...));
}

namespace di::concepts {
template<typename F, typename R, typename... Args>
concept InvocableTo =
    Invocable<F, Args...> && (LanguageVoid<R> || ImplicitlyConvertibleTo<meta::InvokeResult<F, Args...>, R>);

template<typename R, typename... Ts>
concept InvocableR = Invocable<Ts...> && (LanguageVoid<R> || ImplicitlyConvertibleTo<meta::InvokeResult<Ts...>, R>);
}

namespace di::function {
namespace detail {
    struct InvokeFunction {
        template<typename F, typename... Args>
        requires(concepts::Invocable<F, Args...>)
        constexpr auto operator()(F&& f, Args&&... args) const -> meta::InvokeResult<F, Args...> {
            return function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
        }
    };

    template<typename R>
    struct InvokeRFunction {
        template<typename F, typename... Args>
        requires(concepts::InvocableTo<F, R, Args...>)
        constexpr auto operator()(F&& f, Args&&... args) const -> R {
            if constexpr (concepts::LanguageVoid<R>) {
                (void) function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
            } else {
                return function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
            }
        }
    };
}

constexpr inline auto invoke = function::detail::InvokeFunction {};

template<typename R>
constexpr inline auto invoke_r = function::detail::InvokeRFunction<R> {};
}

namespace di {
using concepts::Invocable;
using concepts::InvocableTo;
using function::invoke;
using function::invoke_r;
using meta::InvokeResult;
}
