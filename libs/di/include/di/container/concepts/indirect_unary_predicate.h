#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/iterator_common_reference.h>
#include <di/meta/operations.h>
#include <di/meta/relation.h>

namespace di::concepts {
template<typename F, typename Iter>
concept IndirectUnaryPredicate =
    IndirectlyReadable<Iter> && CopyConstructible<F> && Predicate<F&, meta::IteratorValue<Iter>> &&
    Predicate<F&, meta::IteratorReference<Iter>> && Predicate<F&, meta::IteratorCommonReference<Iter>>;
}
