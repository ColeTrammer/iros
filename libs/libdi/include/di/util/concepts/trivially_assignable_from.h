#pragma once

namespace di::util::concepts {
template<typename T, typename U>
concept TriviallyAssignableFrom = __is_trivially_assignable(T, U);
}
