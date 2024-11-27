#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::util {
namespace detail {
    template<template<typename...> typename Template, typename... Args>
    concept CTADDeducible = requires(Args&&... args) { Template(util::forward<Args>(args)...); };

    struct DeduceCreateFunction {
        template<template<typename...> typename Template, typename... Args>
        requires(CTADDeducible<Template, Args...>)
        constexpr auto operator()(InPlaceTemplate<Template>, Args&&... args) const
            -> decltype(Template(util::forward<Args>(args)...));

        template<template<typename...> typename Template, typename... Args>
        requires(!CTADDeducible<Template, Args...>)
        constexpr auto operator()(InPlaceTemplate<Template>, Args&&...) const
            -> meta::TagInvokeResult<DeduceCreateFunction, InPlaceTemplate<Template>, Args...>;
    };
}

constexpr inline auto deduce_create = detail::DeduceCreateFunction {};
}

namespace di::concepts {
template<template<typename...> typename Template, typename... Args>
concept CreateDeducible = requires { util::deduce_create(in_place_template<Template>, util::declval<Args>()...); };
}

namespace di::meta {
template<template<typename...> typename Template, typename... Args>
requires(concepts::CreateDeducible<Template, Args...>)
using DeduceCreate = decltype(util::deduce_create(in_place_template<Template>, util::declval<Args>()...));
}
