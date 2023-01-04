#pragma once

#include <di/meta/type_constant.h>
#include <di/vocab/variant/variant_forward_declaration.h>

namespace di::meta {
namespace detail {
    struct EmptyVariant {
        EmptyVariant() = delete;
    };

    template<typename... Types>
    struct VariantOrEmptyHelper : TypeConstant<vocab::Variant<Types...>> {};

    template<>
    struct VariantOrEmptyHelper<> : TypeConstant<EmptyVariant> {};
}

template<typename... Types>
using VariantOrEmpty = detail::VariantOrEmptyHelper<Types...>::Type;
}