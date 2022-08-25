#pragma once

namespace di::util::concepts {
template<typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);
}
