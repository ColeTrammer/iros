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
