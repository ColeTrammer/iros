#pragma once

namespace di::container {
struct IntrusiveCallbackBase;

struct DefaultIntrusiveListTag;

template<typename Tag = DefaultIntrusiveListTag>
class IntrusiveListNode;

template<typename T, typename Tag = DefaultIntrusiveListTag, typename Self = Void>
class IntrusiveList;
}
