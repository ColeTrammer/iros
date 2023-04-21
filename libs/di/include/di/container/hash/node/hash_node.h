#pragma once

#include <di/container/intrusive/forward_list_node.h>

namespace di::container {
template<typename Tag>
struct HashNode : IntrusiveForwardListNode<Tag> {};
}
