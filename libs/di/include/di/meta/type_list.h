#pragma once

#include <di/meta/type_constant.h>
#include <di/types/size_t.h>

namespace di::meta {
namespace detail {
    template<types::size_t index, typename... Types>
    struct TypeAtIndex {};

    template<typename T, typename... Rest>
    struct TypeAtIndex<0, T, Rest...> : TypeConstant<T> {};

    template<types::size_t index, typename T, typename... Rest>
    struct TypeAtIndex<index, T, Rest...> : TypeAtIndex<index - 1, Rest...> {};
}

template<typename... Types>
struct TypeList {
    template<types::size_t index>
    using TypeAtIndex = detail::TypeAtIndex<index, Types...>::Type;
};
}
