#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::util {
namespace detail {
    struct DeduceCreateFunction {
        template<template<typename...> typename Template, typename... Args>
        constexpr meta::TagInvokeResult<DeduceCreateFunction, InPlaceTemplate<Template>, Args...> operator()(InPlaceTemplate<Template>,
                                                                                                             Args&&...) const;
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
