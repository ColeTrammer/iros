#pragma once

#include <di/concepts/conjunction.h>
#include <di/meta/bool_constant.h>
#include <di/meta/list/invoke.h>
#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename Pred, typename Type>
    struct AllHelper {};

    template<typename Pred, typename... Types>
    struct AllHelper<Pred, List<Types...>> : BoolConstant<concepts::Conjunction<meta::Invoke<Pred, Types> {}...>> {};
}

template<concepts::TypeList List, concepts::MetaInvocable Pred>
constexpr inline bool All = detail::AllHelper<Pred, List>::value;
}
