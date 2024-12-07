#pragma once

#include "di/types/prelude.h"

namespace di::container {
struct DefaultIntrusiveListTag;

template<typename Tag = DefaultIntrusiveListTag>
class IntrusiveListNode;

template<typename T, typename Tag = DefaultIntrusiveListTag, typename Self = Void>
class IntrusiveList;
}
