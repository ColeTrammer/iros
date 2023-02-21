#pragma once

#include <di/meta/conditional.h>
#include <di/meta/list/fold.h>
#include <di/meta/list/invoke.h>
#include <di/meta/list/push_back.h>

namespace di::meta {
namespace detail {
    template<typename Pred>
    struct FilterReducer {
        template<typename Acc, typename Val>
        using Invoke = meta::Conditional<meta::Invoke<Pred, Val>::value, meta::PushBack<Acc, Val>, Acc>;
    };
}

template<concepts::TypeList List, concepts::MetaInvocable Pred>
using Filter = meta::Fold<List, meta::List<>, detail::FilterReducer<Pred>>;
}
