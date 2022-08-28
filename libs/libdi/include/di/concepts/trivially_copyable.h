#pragma once

namespace di::concepts {
template<typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);
}
