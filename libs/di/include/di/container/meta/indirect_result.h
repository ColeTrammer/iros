#pragma once

#include "di/container/concepts/indirectly_readable.h"
#include "di/container/meta/iterator_reference.h"
#include "di/function/invoke.h"

namespace di::meta {
template<typename F, concepts::IndirectlyReadable... Its>
requires(concepts::Invocable<F, meta::IteratorReference<Its>...>)
using IndirectResult = InvokeResult<F, meta::IteratorReference<Its>...>;
}
