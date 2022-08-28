#pragma once

namespace di::concepts {
template<typename T>
concept Enum = __is_enum(T);
}
