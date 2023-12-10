#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/vocab/variant/variant_forward_declaration.h>

namespace di::meta {
namespace detail {
    struct EmptyVariant {
        EmptyVariant() = delete;
    };

    template<typename List>
    struct VariantOrEmptyHelper;

    template<typename... Types>
    struct VariantOrEmptyHelper<List<Types...>> : TypeConstant<vocab::Variant<Types...>> {};

    template<>
    struct VariantOrEmptyHelper<List<>> : TypeConstant<EmptyVariant> {};
}

template<typename... Types>
using VariantOrEmpty = detail::VariantOrEmptyHelper<meta::Unique<meta::List<Types...>>>::Type;
}
