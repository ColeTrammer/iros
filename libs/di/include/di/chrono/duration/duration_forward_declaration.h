#pragma once

#include <di/math/rational/ratio.h>

namespace di::chrono {
template<typename Rep, math::detail::IsRatio Period = math::Ratio<1>>
class Duration;
}
