#pragma once

#include <di/concepts/constructible_from.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>

namespace di::container {
struct LinkedListNode {
    LinkedListNode* next { nullptr };
    LinkedListNode* prev { nullptr };
};

template<typename T>
struct ConcreteLinkedListNode : LinkedListNode {
    T value;

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit ConcreteLinkedListNode(InPlace, Args&&... args) : value(util::forward<Args>(args)...) {}
};
}