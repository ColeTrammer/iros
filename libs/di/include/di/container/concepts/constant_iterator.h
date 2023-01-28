#pragma once

#include <di/concepts/same_as.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/meta/iterator_const_reference.h>
#include <di/container/meta/iterator_reference.h>

namespace di::concepts {
template<typename Iter>
concept ConstantIterator =
    InputIterator<Iter> && SameAs<meta::IteratorConstReference<Iter>, meta::IteratorReference<Iter>>;
}