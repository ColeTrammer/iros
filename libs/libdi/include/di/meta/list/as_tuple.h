#pragma once

#include <di/meta/list/list.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AsTupleHelper {};

    template<typename... Types>
    struct AsTupleHelper<List<Types...>> : TypeConstant<vocab::Tuple<Types...>> {};
}

template<concepts::TypeList T>
using AsTuple = detail::AsTupleHelper<T>::Type;
}