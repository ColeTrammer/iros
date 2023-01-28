#include <liim/format.h>
#include <liim/option.h>
#include <liim/span.h>
#include <unicode/property_range.h>

namespace Unicode::Detail {
Option<uint32_t> find_range_for_code_point(Span<const PropertyRange> ranges, uint32_t code_point) {
    size_t lo = 0;
    size_t hi = ranges.size();

    auto is_in_range = [&](const PropertyRange& range) -> bool {
        return range.start <= code_point && code_point <= range.end_inclusive;
    };

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        auto& range = ranges[mid];

        if (is_in_range(range)) {
            return range.value;
        } else if (code_point < range.start) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }

    auto& range = ranges[lo];
    if (is_in_range(range)) {
        return range.value;
    }
    return None {};
}
}
