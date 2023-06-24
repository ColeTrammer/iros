#pragma once

#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/container/intrusive/list_forward_declaration.h>
#include <di/util/immovable.h>

namespace di::container {
template<typename Tag>
class IntrusiveListNode : util::Immovable {
public:
    constexpr IntrusiveListNode() : next(nullptr), prev(nullptr) {}

private:
    template<typename, typename, typename>
    friend class IntrusiveList;

    constexpr IntrusiveListNode(IntrusiveListNode* next_, IntrusiveListNode* prev_) : next(next_), prev(prev_) {}

    IntrusiveListNode* next { nullptr };
    IntrusiveListNode* prev { nullptr };
};
}

namespace di {
using container::IntrusiveListNode;
}
