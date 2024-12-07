#pragma once

#include "di/types/prelude.h"
#include "di/util/create_in_place.h"
#include "di/util/deduce_create.h"
#include "di/util/forward.h"

namespace di::concepts {
template<typename T, typename... Args>
concept CreatableFrom =
    requires(Args&&... args) { util::create_in_place(in_place_type<T>, util::forward<Args>(args)...); };

template<template<typename...> typename Template, typename... Args>
concept TemplateCreatableFrom =
    CreateDeducible<Template, Args...> && CreatableFrom<meta::DeduceCreate<Template, Args...>, Args...>;
}

namespace di::util {
template<typename T, typename... Args>
requires(concepts::LanguageVoid<T> || concepts::CreatableFrom<T, Args...>)
constexpr auto create(Args&&... args) {
    if constexpr (!concepts::LanguageVoid<T>) {
        return create_in_place(in_place_type<T>, util::forward<Args>(args)...);
    }
}

template<template<typename...> typename Template, typename... Args>
requires(concepts::TemplateCreatableFrom<Template, Args...>)
constexpr auto create(Args&&... args) {
    return create_in_place(in_place_type<meta::DeduceCreate<Template, Args...>>, util::forward<Args>(args)...);
}
}

namespace di {
using util::create;
}
