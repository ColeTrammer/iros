#pragma once

#include <di/concepts/same_as.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::concepts {
template<typename Sent, typename Iter>
concept SizedSentinelFor = SentinelFor<Sent, Iter> && requires(Iter const& iterator, Sent const& sentinel) {
    { sentinel - iterator } -> SameAs<meta::IteratorSSizeType<Iter>>;
    { iterator - sentinel } -> SameAs<meta::IteratorSSizeType<Iter>>;
};
}
