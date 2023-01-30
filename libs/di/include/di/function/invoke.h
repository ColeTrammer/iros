#pragma once

#include <di/concepts/base_of.h>
#include <di/concepts/implicitly_convertible_to.h>
#include <di/concepts/member_function_pointer.h>
#include <di/concepts/member_object_pointer.h>
#include <di/concepts/member_pointer.h>
#include <di/concepts/reference_wrapper.h>
#include <di/meta/decay.h>
#include <di/meta/member_pointer_class.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

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
concept InvocableTo = Invocable<F, Args...> &&
                      (LanguageVoid<R> || ImplicitlyConvertibleTo<meta::InvokeResult<F, Args...>, R>);

template<typename R, typename... Ts>
concept InvocableR = Invocable<Ts...> && (LanguageVoid<R> || ImplicitlyConvertibleTo<meta::InvokeResult<Ts...>, R>);
}

namespace di::function {
template<typename F, typename... Args>
requires(concepts::Invocable<F, Args...>)
constexpr meta::InvokeResult<F, Args...> invoke(F&& f, Args&&... args) {
    return function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
}

template<typename R, typename F, typename... Args>
requires(concepts::InvocableTo<F, R, Args...>)
constexpr R invoke_r(F&& f, Args&&... args) {
    if constexpr (concepts::LanguageVoid<R>) {
        (void) function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
    } else {
        return function::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
    }
}
}
