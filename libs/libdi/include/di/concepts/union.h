#pragma once

namespace di::concepts {
template<typename T>
concept Union = __is_union(T);
}
