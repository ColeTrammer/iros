#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/strict_weak_order.h>
#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>

namespace di::concepts {
template<typename F, typename It, typename Jt = It>
concept IndirectStrictWeakOrder = IndirectlyReadable<It> && IndirectlyReadable<Jt> && CopyConstructible<F> &&
                                  StrictWeakOrder<F&, meta::IteratorValue<It>&, meta::IteratorValue<Jt>&> &&
                                  StrictWeakOrder<F&, meta::IteratorValue<It>&, meta::IteratorReference<Jt>> &&
                                  StrictWeakOrder<F&, meta::IteratorReference<It>, meta::IteratorValue<Jt>&> &&
                                  StrictWeakOrder<F&, meta::IteratorReference<It>, meta::IteratorReference<Jt>>;
}