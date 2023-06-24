#pragma once

#include <di/container/types/random_access_iterator_tag.h>

namespace di::types {
struct ContiguousIteratorTag : RandomAccessIteratorTag {};
}

namespace di {
using types::ContiguousIteratorTag;
}
