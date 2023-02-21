#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/conditional.h>
#include <di/meta/list/fold.h>
#include <di/meta/list/push_back.h>

namespace di::meta {
namespace detail {
    template<typename Needle, typename Replacement>
    struct ReplaceReducer {
        template<typename Acc, typename Val>
        using Invoke = meta::PushBack<Acc, meta::Conditional<concepts::SameAs<Val, Needle>, Replacement, Val>>;
    };
}

template<concepts::TypeList List, typename Needle, typename Replacement>
using Replace = meta::Fold<List, meta::List<>, detail::ReplaceReducer<Needle, Replacement>>;
}
