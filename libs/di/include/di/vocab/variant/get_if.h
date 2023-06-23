#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/remove_rvalue_reference.h>
#include <di/util/get.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_types.h>
#include <di/vocab/variant/variant_value.h>

namespace di::vocab {
template<size_t index, concepts::VariantLike Var, typename Res = meta::VariantValue<Var, index>>
constexpr Optional<meta::RemoveRValueReference<Res>> get_if(Var&& variant) {
    if (variant_index(variant) != index) {
        return nullopt;
    }
    return util::get<index>(util::forward<Var>(variant));
}

template<typename T, concepts::VariantLike Var, typename List = meta::VariantTypes<Var>,
         auto index = meta::Lookup<T, List>, typename Res = meta::VariantValue<Var, index>>
requires(meta::UniqueType<T, List>)
constexpr Optional<meta::RemoveRValueReference<Res>> get_if(Var&& variant) {
    if (variant_index(variant) != index) {
        return nullopt;
    }
    return util::get<index>(util::forward<Var>(variant));
}
}
