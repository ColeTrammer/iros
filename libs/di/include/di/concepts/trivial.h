#pragma once

namespace di::concepts {
template<typename T>
concept Trivial = __is_trivial(T);
}
