#pragma once

#include <di/types/prelude.h>

namespace di::container {
struct DefaultIntrusiveForwardListTag;

template<typename Tag = DefaultIntrusiveForwardListTag>
class IntrusiveForwardListNode;

template<typename T, typename Tag = DefaultIntrusiveForwardListTag, typename Self = Void>
class IntrusiveForwardList;
}
