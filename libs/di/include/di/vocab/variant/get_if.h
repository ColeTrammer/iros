#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/language.h>
#include <di/util/get.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_types.h>
#include <di/vocab/variant/variant_value.h>

namespace di::vocab {
template<size_t index, concepts::VariantLike Var, typename Res = meta::VariantValue<Var, index>>
constexpr auto get_if(Var&& variant) -> Optional<meta::RemoveRValueReference<Res>> {
    if (variant_index(variant) != index) {
        return nullopt;
    }
    return util::get<index>(util::forward<Var>(variant));
}

template<typename T, concepts::VariantLike Var, typename List = meta::VariantTypes<Var>,
         auto index = meta::Lookup<T, List>, typename Res = meta::VariantValue<Var, index>>
requires(meta::UniqueType<T, List>)
constexpr auto get_if(Var&& variant) -> Optional<meta::RemoveRValueReference<Res>> {
    if (variant_index(variant) != index) {
        return nullopt;
    }
    return util::get<index>(util::forward<Var>(variant));
}
}

namespace di {
using vocab::get_if;
}
