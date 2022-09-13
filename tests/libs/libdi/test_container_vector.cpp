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

    v.clear();
    EXPECT(v.empty());
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

constexpr void to() {
    auto v = di::create<di::Vector<int>>(di::range(6));
    EXPECT(v.size() == 6u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[4], 4);

    auto w = di::container::to<di::Vector<int>>(di::range(6));
    EXPECT(w.size() == 6u);
    EXPECT_EQ(w[0], 0);
    EXPECT_EQ(w[4], 4);

    auto x = di::create<di::Vector>(di::range(6));
    EXPECT(x.size() == 6u);
    EXPECT_EQ(x[0], 0);
    EXPECT_EQ(x[4], 4);

    auto y = di::container::to<di::Vector>(di::range(6));
    EXPECT(y.size() == 6u);
    EXPECT_EQ(y[0], 0);
    EXPECT_EQ(y[4], 4);

    auto u = di::range(6) | di::container::to<di::Vector<int>>();
    EXPECT(u.size() == 6u);
    EXPECT_EQ(u[0], 0);
    EXPECT_EQ(u[4], 4);

    auto z = di::range(6) | di::container::to<di::Vector>();
    EXPECT(z.size() == 6u);
    EXPECT_EQ(z[0], 0);
    EXPECT_EQ(z[4], 4);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Vector>(), di::range)) | di::container::to<di::Vector>();
    EXPECT(a.size() == 6u);
    EXPECT(a[0].size() == 0);
    EXPECT(a[4].size() == 4u);
}

constexpr void clone() {
    auto v = di::range(6) | di::container::to<di::Vector>();
    EXPECT(v.size() == 6);

    auto w = di::clone(v);
    EXPECT(w.size() == 6);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Vector>(), di::range)) | di::container::to<di::Vector>();
    EXPECT(a.size() == 6u);
    EXPECT(a[0].size() == 0);
    EXPECT(a[4].size() == 4u);

    auto b = di::clone(a);
    EXPECT(b.size() == 6u);
    EXPECT(b[0].size() == 0);
    EXPECT(b[4].size() == 4u);
}

TEST_CONSTEXPR(container_vector, basic, basic)
TEST_CONSTEXPR(container_vector, move_only, move_only)
TEST_CONSTEXPR(container_vector, to, to)
TEST_CONSTEXPR(container_vector, clone, clone)
