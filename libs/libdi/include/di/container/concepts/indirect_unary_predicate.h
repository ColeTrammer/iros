#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/predicate.h>
#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_value.h>

namespace di::concepts {
template<typename F, typename Iter>
concept IndirectUnaryPredicate =
    IndirectlyReadable<Iter> && CopyConstructible<F> && Predicate<F&, meta::IteratorValue<Iter>> &&
    Predicate<F&, meta::IteratorReference<Iter>>;
}