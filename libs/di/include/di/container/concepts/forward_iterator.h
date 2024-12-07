#pragma once

#include "di/container/concepts/input_iterator.h"
#include "di/container/concepts/sentinel_for.h"
#include "di/container/types/forward_iterator_tag.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename Iter>
concept ForwardIterator = InputIterator<Iter> && DerivedFrom<meta::IteratorCategory<Iter>, types::ForwardIteratorTag> &&
                          SentinelFor<Iter, Iter> && Regular<Iter> && requires(Iter iter) {
                              { iter++ } -> SameAs<Iter>;
                          };
}
