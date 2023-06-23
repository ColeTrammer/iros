#pragma once

#include <di/meta/common_comparison_category.h>
#include <di/meta/core.h>

namespace di::concepts::detail {
template<typename T, typename Category>
concept ComparesAs = SameAs<meta::CommonComparisonCategory<T, Category>, Category>;
}
