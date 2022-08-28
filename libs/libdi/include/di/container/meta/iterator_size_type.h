#pragma once

#include <di/container/meta/iterator_ssize_type.h>
#include <di/meta/make_unsigned.h>

namespace di::meta {
template<typename T>
using IteratorSizeType = MakeUnsigned<IteratorSSizeType<T>>;
}
