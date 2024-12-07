#pragma once

#include "di/container/concepts/indirectly_readable.h"
#include "di/container/meta/iterator_common_reference.h"
#include "di/container/meta/iterator_reference.h"
#include "di/container/meta/iterator_value.h"
#include "di/meta/operations.h"
#include "di/meta/relation.h"

namespace di::concepts {
template<typename F, typename It, typename Jt = It>
concept IndirectStrictWeakOrder =
    IndirectlyReadable<It> && IndirectlyReadable<Jt> && CopyConstructible<F> &&
    StrictWeakOrder<F&, meta::IteratorValue<It>&, meta::IteratorValue<Jt>&> &&
    StrictWeakOrder<F&, meta::IteratorValue<It>&, meta::IteratorReference<Jt>> &&
    StrictWeakOrder<F&, meta::IteratorReference<It>, meta::IteratorValue<Jt>&> &&
    StrictWeakOrder<F&, meta::IteratorReference<It>, meta::IteratorReference<Jt>> &&
    StrictWeakOrder<F&, meta::IteratorCommonReference<It>, meta::IteratorCommonReference<Jt>>;

template<typename F, typename It, typename Jt = It>
concept IndirectStrictPartialOrder =
    IndirectlyReadable<It> && IndirectlyReadable<Jt> && CopyConstructible<F> &&
    StrictPartialOrder<F&, meta::IteratorValue<It>&, meta::IteratorValue<Jt>&> &&
    StrictPartialOrder<F&, meta::IteratorValue<It>&, meta::IteratorReference<Jt>> &&
    StrictPartialOrder<F&, meta::IteratorReference<It>, meta::IteratorValue<Jt>&> &&
    StrictPartialOrder<F&, meta::IteratorReference<It>, meta::IteratorReference<Jt>> &&
    StrictPartialOrder<F&, meta::IteratorCommonReference<It>, meta::IteratorCommonReference<Jt>>;
}
