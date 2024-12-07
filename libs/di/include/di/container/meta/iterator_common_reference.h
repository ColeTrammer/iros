#pragma once

#include "di/container/concepts/indirectly_readable.h"
#include "di/container/meta/iterator_reference.h"
#include "di/container/meta/iterator_value.h"
#include "di/meta/common.h"

namespace di::meta {
template<concepts::IndirectlyReadable T>
using IteratorCommonReference = meta::CommonReference<meta::IteratorReference<T>, meta::IteratorValue<T>&>;
}
