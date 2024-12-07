#pragma once

#include "di/container/concepts/input_iterator.h"
#include "di/container/meta/iterator_const_reference.h"
#include "di/container/meta/iterator_reference.h"
#include "di/meta/core.h"

namespace di::concepts {
template<typename Iter>
concept ConstantIterator =
    InputIterator<Iter> && SameAs<meta::IteratorConstReference<Iter>, meta::IteratorReference<Iter>>;
}
