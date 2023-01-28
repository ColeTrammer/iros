#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>
#include <di/function/invoke.h>

namespace di::concepts {
template<typename F, typename It>
concept IndirectlyUnaryInvocable =
    IndirectlyReadable<It> && CopyConstructible<F> && Invocable<F&, meta::IteratorValue<It>&> &&
    Invocable<F&, meta::IteratorReference<It>>;
}