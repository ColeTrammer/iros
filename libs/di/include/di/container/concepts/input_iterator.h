#pragma once

#include "di/container/concepts/iterator.h"
#include "di/container/types/input_iterator_tag.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename Iter>
concept InputIterator = Iterator<Iter> && DerivedFrom<meta::IteratorCategory<Iter>, types::InputIteratorTag>;
}
