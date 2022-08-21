#pragma once

namespace di::util::concepts {
template<typename T>
concept Class = __is_class(T);
}
