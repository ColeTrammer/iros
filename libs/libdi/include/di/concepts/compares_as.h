#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/common_comparison_category.h>

namespace di::concepts::detail {
template<typename T, typename Category>
concept ComparesAs = SameAs<meta::CommonComparisonCategory<T, Category>, Category>;
}
