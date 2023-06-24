#pragma once

#include <di/container/types/forward_iterator_tag.h>

namespace di::types {
struct BidirectionalIteratorTag : ForwardIteratorTag {};
}

namespace di {
using types::BidirectionalIteratorTag;
}
