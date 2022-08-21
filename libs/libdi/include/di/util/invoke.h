#pragma once

#include <di/util/concepts/base_of.h>
#include <di/util/concepts/implicitly_convertible_to.h>
#include <di/util/concepts/member_function_pointer.h>
#include <di/util/concepts/member_object_pointer.h>
#include <di/util/concepts/member_pointer.h>
#include <di/util/concepts/reference_wrapper.h>
#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/meta/decay.h>
#include <di/util/meta/member_pointer_class.h>

namespace di::util {
namespace detail {
    template<concepts::MemberFunctionPointer F, typename FirstArg, typename... Args>
    requires(concepts::BaseOf<meta::MemberPointerClass<F>, meta::Decay<FirstArg>>)
    constexpr auto invoke_impl(F f, FirstArg&& first_arg, Args&&... args)
        -> decltype((util::forward<FirstArg>(first_arg).*f)(util::forward<Args>(args)...)) {
        return (util::forward<FirstArg>(first_arg).*f)(util::forward<Args>(args)...);
    }

    template<concepts::MemberFunctionPointer F, typename FirstArg, typename... Args>
    requires(concepts::ReferenceWrapper<meta::Decay<FirstArg>>)
    constexpr auto invoke_impl(F f, FirstArg&& first_arg, Args&&... args) -> decltype((first_arg.get().*f)(util::forward<Args>(args)...)) {
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

namespace concepts {
    template<typename F, typename... Args>
    concept Invocable =
        requires(F&& f, Args&&... args) { di::util::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...); };
}

namespace meta {
    template<typename F, typename... Args>
    requires(concepts::Invocable<F, Args...>)
    using InvokeResult = decltype(di::util::detail::invoke_impl(util::declval<F>(), util::declval<Args>()...));
}

namespace concepts {
    template<typename F, typename R, typename... Args>
    concept InvocableTo = Invocable<F, Args...> && ImplicitlyConvertibleTo<meta::InvokeResult<F, Args...>, R>;
}

template<typename F, typename... Args>
requires(concepts::Invocable<F, Args...>)
constexpr meta::InvokeResult<F, Args...> invoke(F&& f, Args&&... args) {
    return di::util::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
}

template<typename R, typename F, typename... Args>
requires(concepts::InvocableTo<F, R, Args...>)
constexpr R invoke_r(F&& f, Args&&... args) {
    if constexpr (concepts::LanguageVoid<R>) {
        di::util::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
    } else {
        return di::util::detail::invoke_impl(util::forward<F>(f), util::forward<Args>(args)...);
    }
}
}
