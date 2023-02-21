#pragma once

#include <di/container/concepts/constant_iterator.h>
#include <di/container/iterator/const_iterator_impl.h>

namespace di::meta {
template<concepts::InputIterator Iter>
using ConstIterator = Conditional<concepts::ConstantIterator<Iter>, Iter, container::ConstIteratorImpl<Iter>>;
}
