#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_common_reference.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>
#include <di/meta/operations.h>
#include <di/meta/relation.h>

namespace di::concepts {
template<typename F, typename It, typename Jt = It>
concept IndirectBinaryPredicate = IndirectlyReadable<It> && IndirectlyReadable<Jt> && CopyConstructible<F> &&
                                  Predicate<F&, meta::IteratorValue<It>&, meta::IteratorValue<Jt>&> &&
                                  Predicate<F&, meta::IteratorValue<It>&, meta::IteratorReference<Jt>> &&
                                  Predicate<F&, meta::IteratorReference<It>, meta::IteratorValue<Jt>&> &&
                                  Predicate<F&, meta::IteratorReference<It>, meta::IteratorReference<Jt>> &&
                                  Predicate<F&, meta::IteratorCommonReference<It>, meta::IteratorReference<Jt>>;
}
