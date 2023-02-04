#pragma once

#include <di/meta/conditional.h>
#include <di/meta/list/fold.h>
#include <di/meta/list/invoke.h>
#include <di/meta/list/push_back.h>

namespace di::meta {
namespace detail {
    template<typename Pred, typename Replacement>
    struct ReplaceIfReducer {
        template<typename Acc, typename Val>
        using Invoke = meta::PushBack<Acc, meta::Conditional<meta::Invoke<Pred, Val>::value, Replacement, Val>>;
    };
}

template<concepts::TypeList List, concepts::MetaInvocable Pred, typename Replacement>
using ReplaceIf = meta::Fold<List, meta::List<>, detail::ReplaceIfReducer<Pred, Replacement>>;
}