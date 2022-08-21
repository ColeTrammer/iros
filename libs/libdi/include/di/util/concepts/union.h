#pragma once

namespace di::util::concepts {
template<typename T>
concept Union = __is_union(T);
}
