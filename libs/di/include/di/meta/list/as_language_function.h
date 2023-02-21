#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename R, typename List>
    struct AsLanguageFunction;

    template<typename R, typename... Types>
    struct AsLanguageFunction<R, List<Types...>> : TypeConstant<R(Types...)> {};
}

template<typename R, concepts::TypeList T>
using AsLanguageFunction = detail::AsLanguageFunction<R, T>::Type;
}
