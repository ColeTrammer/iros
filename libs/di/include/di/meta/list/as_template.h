#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<template<typename...> typename Template, typename List>
    struct AsTemplateHelper {};

    template<template<typename...> typename Template, typename... Types>
    struct AsTemplateHelper<Template, List<Types...>> : TypeConstant<Template<Types...>> {};
}

template<template<typename...> typename Template, concepts::TypeList T>
using AsTemplate = detail::AsTemplateHelper<Template, T>::Type;
}
