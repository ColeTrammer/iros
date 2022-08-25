#pragma once

namespace di::util::concepts {
template<typename T, typename... Args>
concept TriviallyConstructibleFrom = __is_trivially_constructible(T, Args...);
}
