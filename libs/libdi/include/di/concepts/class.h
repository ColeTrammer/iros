#pragma once

namespace di::concepts {
template<typename T>
concept Class = __is_class(T);
}
