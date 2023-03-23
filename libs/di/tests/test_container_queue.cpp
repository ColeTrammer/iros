#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void priority_queue_basic() {
    auto x = di::PriorityQueue<i32> {};
    x.push(3);
    x.push(1);
    x.push(5);

    ASSERT_EQ(x.size(), 3u);
    ASSERT_EQ(x.pop(), 5);
    ASSERT_EQ(x.pop(), 3);
    ASSERT_EQ(x.pop(), 1);
    ASSERT(x.empty());
}

constexpr void priority_queue_to() {
    auto x = di::range(5) | di::to<di::PriorityQueue>();

    ASSERT_EQ(x.pop(), 4);
    ASSERT_EQ(x.pop(), 3);
    ASSERT_EQ(x.pop(), 2);
    ASSERT_EQ(x.pop(), 1);
    ASSERT_EQ(x.pop(), 0);

    auto y = di::range(100) | di::to<di::PriorityQueue>();
    ASSERT(di::container::equal(y, di::range(100) | di::reverse));

    auto z = di::range(100) | di::reverse | di::to<di::PriorityQueue>(di::compare_backwards);
    ASSERT(di::container::equal(z, di::range(100)));
}

constexpr void stack() {
    auto a = di::Stack<int> {};

    a.push(1);
    a.push(2);
    a.push(3);

    ASSERT_EQ(a.size(), 3u);
    ASSERT_EQ(a.pop(), 3);
    ASSERT_EQ(a.pop(), 2);
    ASSERT_EQ(a.pop(), 1);
    ASSERT(a.empty());

    auto b = di::range(5) | di::to<di::Stack>();
    ASSERT(di::is_sorted(b | di::to<di::Vector>(), di::compare_backwards));
    ASSERT(b.empty());
}

constexpr void queue() {
    auto queue = di::Queue<int> {};
    queue.push(1);
    queue.push(2);
    queue.push(3);

    ASSERT_EQ(queue.size(), 3u);
    ASSERT_EQ(queue.pop(), 1);
    ASSERT_EQ(queue.pop(), 2);
    ASSERT_EQ(queue.pop(), 3);
    ASSERT(queue.empty());

    auto b = di::range(5) | di::to<di::Queue>();

    ASSERT(di::is_sorted(b | di::to<di::Vector>()));
    ASSERT(b.empty());
}

TESTC(container_queue, priority_queue_basic)
TESTC(container_queue, priority_queue_to)
TESTC(container_queue, stack)
TESTC_CLANG(container_queue, queue)
