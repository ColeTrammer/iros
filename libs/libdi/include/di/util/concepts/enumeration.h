#pragma once

namespace di::util::concepts {
template<typename T>
concept Enumeration = __is_enum(T);
}
