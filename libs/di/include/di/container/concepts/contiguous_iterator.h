#pragma once

#include "di/container/concepts/random_access_iterator.h"
#include "di/container/types/contiguous_iterator_tag.h"
#include "di/meta/language.h"

namespace di::concepts {
template<typename Iter>
concept ContiguousIterator =
    RandomAccessIterator<Iter> && DerivedFrom<meta::IteratorCategory<Iter>, types::ContiguousIteratorTag> &&
    LValueReference<meta::IteratorReference<Iter>>;
}
