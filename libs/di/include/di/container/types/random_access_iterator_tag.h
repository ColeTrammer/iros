#pragma once

#include <di/container/types/bidirectional_iterator_tag.h>

namespace di::types {
struct RandomAccessIteratorTag : BidirectionalIteratorTag {};
}

namespace di {
using types::RandomAccessIteratorTag;
}
