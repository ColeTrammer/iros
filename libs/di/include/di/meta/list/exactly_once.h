#pragma once

#include <di/meta/list/count.h>

namespace di::meta {
template<typename List, typename T>
concept ExactlyOnce = concepts::TypeList<List> && Count<List, T> == 1;
}
