#pragma once

#include <di/util/concepts/same_as.h>
#include <di/util/meta/common_comparison_category.h>

namespace di::util::concepts::detail {
template<typename T, typename Category>
concept ComparesAs = SameAs<meta::CommonComparisonCategory<T, Category>, Category>;
}
