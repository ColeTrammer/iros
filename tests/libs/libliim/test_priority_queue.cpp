#include <liim/container/priority_queue.h>
#include <test/test.h>

constexpr void basic() {
    auto x = make_priority_queue({ 1, 2, 3, 4, 5 });
    EXPECT_EQ(x.size(), 5u);
    EXPECT_EQ(x.top(), 5);
    x.push(8);
    EXPECT_EQ(x.pop(), 8);
    EXPECT_EQ(x.pop(), 5);
    EXPECT_EQ(x.pop(), 4);
    EXPECT_EQ(x.pop(), 3);
    EXPECT_EQ(x.pop(), 2);
    EXPECT_EQ(x.pop(), 1);
}

constexpr void containers() {
    auto x = collect_priority_queue(reversed(range(5, 10)));
    EXPECT_EQ(collect_vector(reversed(range(5, 10))), collect_vector(move(x)));
    EXPECT(x.empty());
}

constexpr void comparator() {
    auto y = collect_priority_queue(range(3, 10), [](auto x, auto y) {
        return x > y;
    });
    EXPECT_EQ(collect_vector(range(3, 10)), collect_vector(move(y)));
    EXPECT(y.empty());

    auto z = make_priority_queue({ 3, 4, 5 }, Greater<int> {});
    EXPECT_EQ(make_vector({ 3, 4, 5 }), collect_vector(z.clone()));

    auto x = make_priority_queue<long, Greater<long>>({ 1, 2, 3 });
    EXPECT_EQ(make_vector<long>({ 1, 2, 3 }), collect_vector(x.clone()));
}

constexpr void stress() {
    auto x = collect_priority_queue(range(50, 200));
    auto prev = 501;
    for (auto z : x) {
        EXPECT(z < prev);
        prev = z;
    }
    EXPECT(x.empty());

    auto y = collect_priority_queue(repeat(120, 5));
    for (auto z : y) {
        EXPECT_EQ(z, 5);
    }
    EXPECT(y.empty());
}

TEST_CONSTEXPR(priority_queue, basic, basic)
TEST_CONSTEXPR(priority_queue, containers, containers)
TEST_CONSTEXPR(priority_queue, comparator, comparator)
TEST_CONSTEXPR(priority_queue, stress, stress)