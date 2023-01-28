#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct TupleHelper : meta::FalseType {};

    template<typename... Types>
    struct TupleHelper<di::vocab::Tuple<Types...>> : meta::TrueType {};
}

template<typename T>
concept Tuple = detail::TupleHelper<meta::RemoveCVRef<T>>::value;
}
