#include <di/container/vector/prelude.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto v = di::Vector<int> {};
    EXPECT(v.empty());

    v.push_back(5);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 5);

    v.push_back(6);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 5);
    EXPECT_EQ(v[1], 6);

    EXPECT(v.pop_back() == 6);
    EXPECT(v.pop_back() == 5);
    EXPECT(v.empty());

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    v.erase(v.iterator(2));
    EXPECT(v.size() == 4);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(v[3], 5);

    v.erase(v.iterator(1), v.iterator(3));
    EXPECT(v.size() == 2);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 5);

    v.insert(v.iterator(1), 3);
    EXPECT(v.size() == 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 5);

    v.resize(6);
    EXPECT(v.size() == 6);
    EXPECT(v.back() == 0);

    v.resize(2);
    EXPECT(v.size() == 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
}

struct M {
    constexpr M(int x_) : x(x_) {}

    constexpr M(M const&) = delete;
    constexpr M(M&& xx) : x(di::exchange(xx.x, 0)) {}

    constexpr ~M() {}

    int x;

    constexpr friend bool operator==(M const& a, M const& b) { return a.x == b.x; }
};

constexpr void move_only() {
    auto v = di::Vector<M> {};
    v.push_back(M { 2 });
    v.push_back(M { 4 });

    EXPECT_EQ(v[0].x, 2);
    EXPECT_EQ(v[1].x, 4);
    EXPECT(v.size() == 2);

    auto w = di::move(v);
    EXPECT_EQ(w.size(), 2u);
    EXPECT(v.empty());
}

TEST_CONSTEXPR(container_vector, basic, basic)
TEST_CONSTEXPR(container_vector, move_only, move_only)
