#pragma once

#include <liim/forward.h>
#include <stdint.h>

namespace Unicode::Detail {
struct PropertyRange {
    uint32_t start { 0 };
    uint32_t end_inclusive { 0 };
    uint32_t value { 0 };
};

Option<uint32_t> find_range_for_code_point(Span<const PropertyRange> ranges, uint32_t code_point);
}
