#pragma once

#include <di/meta/list/concepts/valid_instantiation.h>
#include <di/meta/list/list.h>
#include <di/meta/list/type.h>

namespace di::meta {
namespace detail {
    template<template<typename...> typename Template, typename List>
    struct AsTemplateHelper {};

    template<template<typename...> typename Template, typename... Types>
    requires(concepts::ValidInstantiation<Template, Types...>)
    struct AsTemplateHelper<Template, List<Types...>> : TypeConstant<Template<Types...>> {};
}

template<template<typename...> typename Template, concepts::TypeList T>
using AsTemplate = Type<detail::AsTemplateHelper<Template, T>>;
}
