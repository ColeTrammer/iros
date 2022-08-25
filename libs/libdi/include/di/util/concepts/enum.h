#pragma once

namespace di::util::concepts {
template<typename T>
concept Enum = __is_enum(T);
}
