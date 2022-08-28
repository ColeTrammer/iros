#pragma once

namespace di::concepts {
template<typename T, typename U>
concept BaseOf = __is_base_of(T, U);
}
