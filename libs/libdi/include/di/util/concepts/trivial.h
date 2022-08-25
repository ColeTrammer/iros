#pragma once

namespace di::util::concepts {
template<typename T>
concept Trivial = __is_trivial(T);
}
