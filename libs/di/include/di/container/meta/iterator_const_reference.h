#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>
#include <di/meta/common_reference.h>

namespace di::meta {
template<concepts ::IndirectlyReadable Iter>
using IteratorConstReference = meta::CommonReference<meta::IteratorValue<Iter> const&&, meta::IteratorReference<Iter>>;
}
