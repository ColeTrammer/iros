#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_common_reference.h>
#include <di/container/meta/iterator_reference.h>
#include <di/meta/operations.h>
#include <di/meta/relation.h>

namespace di::concepts {
template<class F, class It, class Jt = It>
concept IndirectEquivalenceRelation =
    concepts::IndirectlyReadable<It> && concepts::IndirectlyReadable<Jt> && concepts::CopyConstructible<F> &&
    concepts::EquivalenceRelation<F&, meta::IteratorValue<It>&, meta::IteratorValue<Jt>&> &&
    concepts::EquivalenceRelation<F&, meta::IteratorValue<It>&, meta::IteratorReference<Jt>> &&
    concepts::EquivalenceRelation<F&, meta::IteratorReference<It>, meta::IteratorValue<Jt>&> &&
    concepts::EquivalenceRelation<F&, meta::IteratorReference<It>, meta::IteratorReference<Jt>> &&
    concepts::EquivalenceRelation<F&, meta::IteratorCommonReference<It>, meta::IteratorCommonReference<Jt>>;
}
