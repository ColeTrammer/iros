#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void forward_list() {
    struct Node : di::IntrusiveForwardListElement<> {
        constexpr explicit Node(int v) : value(v) {}

        int value;
    };

    auto a = Node(4);
    auto b = Node(6);
    auto c = Node(8);

    auto list = di::IntrusiveForwardList<Node> {};

    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    auto r = list | di::transform([](Node& node) {
                 return node.value;
             }) |
             di::to<di::Vector>();
    auto e = di::Array { 4, 6, 8 } | di::to<di::Vector>();

    ASSERT_EQ(r, e);

    static_assert(di::concepts::InputContainer<decltype(list)>);

    ASSERT_EQ(list.pop_front().transform(&Node::value), 4);
    ASSERT_EQ(list.pop_front().transform(&Node::value), 6);
    ASSERT_EQ(list.pop_front().transform(&Node::value), 8);
    ASSERT_EQ(list.pop_front().transform(&Node::value), di::nullopt);
}

constexpr void list() {
    struct Node : di::IntrusiveListElement<> {
        constexpr explicit Node(int v) : value(v) {}

        int value;
    };

    auto a = Node(4);
    auto b = Node(6);
    auto c = Node(8);

    auto list = di::IntrusiveList<Node> {};

    list.push_back(a);
    list.push_back(b);
    list.push_back(c);

    auto r = list | di::transform([](Node& node) {
                 return node.value;
             }) |
             di::to<di::Vector>();
    auto e = di::Array { 4, 6, 8 } | di::to<di::Vector>();

    ASSERT_EQ(r, e);

    static_assert(di::concepts::InputContainer<decltype(list)>);

    ASSERT_EQ(di::distance(list), 3);
    ASSERT_EQ(list.pop_front().transform(&Node::value), 4);
    ASSERT_EQ(di::distance(list), 2);
    ASSERT_EQ(list.pop_front().transform(&Node::value), 6);
    ASSERT_EQ(di::distance(list), 1);
    ASSERT_EQ(list.pop_front().transform(&Node::value), 8);
    ASSERT_EQ(di::distance(list), 0);
    ASSERT_EQ(list.pop_front().transform(&Node::value), di::nullopt);
}

TESTC(container_intrusive, forward_list)
TESTC(container_intrusive, list)
