#pragma once

#include <di/container/intrusive/forward_list_forward_declaration.h>
#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/util/immovable.h>

namespace di::container {
template<typename Tag>
class IntrusiveForwardListNode : util::Immovable {
public:
    constexpr IntrusiveForwardListNode() : next(nullptr) {}

private:
    template<typename, typename, typename>
    friend class IntrusiveForwardList;

    constexpr IntrusiveForwardListNode(IntrusiveForwardListNode* next_) : next(next_) {}

    IntrusiveForwardListNode* next { nullptr };
};
}

namespace di {
using container::IntrusiveForwardListNode;
}
