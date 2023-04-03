#include <di/container/vector/prelude.h>
#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace container_vector {
constexpr void basic() {
    auto v = di::Vector<int> {};
    ASSERT(v.empty());

    v.push_back(5);
    ASSERT_EQ(v.size(), 1u);
    ASSERT_EQ(v[0], 5);

    v.push_back(6);
    ASSERT_EQ(v.size(), 2u);
    ASSERT_EQ(v[0], 5);
    ASSERT_EQ(v[1], 6);

    ASSERT_EQ(v.pop_back(), 6);
    ASSERT_EQ(v.pop_back(), 5);
    ASSERT(v.empty());

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    v.erase(v.iterator(2));
    ASSERT_EQ(v.size(), 4u);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 4);
    ASSERT_EQ(v[3], 5);

    v.erase(v.iterator(1), v.iterator(3));
    ASSERT_EQ(v.size(), 2u);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 5);

    v.insert(v.iterator(1), 3);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 3);
    ASSERT_EQ(v[2], 5);

    v.resize(6);
    ASSERT_EQ(v.size(), 6u);
    ASSERT_EQ(v.back(), 0);

    v.resize(2);
    ASSERT_EQ(v.size(), 2u);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 3);

    v.clear();
    ASSERT(v.empty());
}

constexpr void reserve() {
    auto v = di::Vector<int> {};
    v.push_back(0);
    v.push_back(1);

    v.reserve(v.capacity() * 2);
    ASSERT_EQ(v, (di::Array { 0, 1 } | di::to<di::Vector>()));
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

    ASSERT_EQ(v[0].x, 2);
    ASSERT_EQ(v[1].x, 4);
    ASSERT_EQ(v.size(), 2u);

    auto w = di::move(v);
    ASSERT_EQ(w.size(), 2u);
    ASSERT(v.empty());
}

constexpr void to() {
    auto v = di::create<di::Vector<int>>(di::range(6));
    ASSERT_EQ(v.size(), 6u);
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[4], 4);

    auto w = di::container::to<di::Vector<int>>(di::range(6));
    ASSERT_EQ(w.size(), 6u);
    ASSERT_EQ(w[0], 0);
    ASSERT_EQ(w[4], 4);

    auto x = di::create<di::Vector>(di::range(6));
    ASSERT_EQ(x.size(), 6u);
    ASSERT_EQ(x[0], 0);
    ASSERT_EQ(x[4], 4);

    auto y = di::container::to<di::Vector>(di::range(6));
    ASSERT_EQ(y.size(), 6u);
    ASSERT_EQ(y[0], 0);
    ASSERT_EQ(y[4], 4);

    auto u = di::range(6) | di::container::to<di::Vector<int>>();
    ASSERT_EQ(u.size(), 6u);
    ASSERT_EQ(u[0], 0);
    ASSERT_EQ(u[4], 4);

    auto z = di::range(6) | di::container::to<di::Vector>();
    ASSERT_EQ(z.size(), 6u);
    ASSERT_EQ(z[0], 0);
    ASSERT_EQ(z[4], 4);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Vector>(), di::range)) |
             di::container::to<di::Vector>();
    ASSERT_EQ(a.size(), 6u);
    ASSERT_EQ(a[0].size(), 0u);
    ASSERT_EQ(a[4].size(), 4u);
}

constexpr void clone() {
    auto v = di::range(6) | di::container::to<di::Vector>();
    ASSERT_EQ(v.size(), 6u);

    auto w = di::clone(v);
    ASSERT_EQ(w.size(), 6u);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Vector>(), di::range)) |
             di::container::to<di::Vector>();
    ASSERT_EQ(a.size(), 6u);
    ASSERT_EQ(a[0].size(), 0u);
    ASSERT_EQ(a[4].size(), 4u);

    auto b = di::clone(a);
    ASSERT_EQ(b.size(), 6u);
    ASSERT_EQ(b[0].size(), 0u);
    ASSERT_EQ(b[4].size(), 4u);
}

constexpr void compare() {
    auto a = di::create<di::Vector>(di::range(6));
    auto b = di::create<di::Vector>(di::range(4));
    auto c = di::create<di::Vector>(di::range(8, 100));
    ASSERT_NOT_EQ(a, b);
    ASSERT_EQ(a, a);
    ASSERT_GT(a, b);
    ASSERT_LT(a, c);
}

constexpr void static_() {
    auto a = di::StaticVector<int, di::meta::SizeConstant<2>> {};
    (void) a.push_back(1);
    (void) a.push_back(2);
    (void) a.push_back(3);

    auto b = di::StaticVector<int, di::meta::SizeConstant<2>> {};
    (void) b.push_back(1);
    (void) b.push_back(2);

    ASSERT_EQ(a, b);

    (void) a.resize(0);
    ASSERT_EQ(a.size(), 0u);

    (void) a.emplace(a.begin());
    (void) a.emplace(a.begin());
    (void) a.emplace(a.begin());
    ASSERT_EQ(a.size(), 2u);

    auto v = di::StaticVector<c8, di::meta::SizeConstant<4>> {};
    (void) v.resize(1);
    v[0] = 9;

    ASSERT_EQ(v.size(), 1u);

    auto w = v;
    (void) w.append_container(di::move(v));

    ASSERT_EQ(w.size(), 2u);
}

TESTC(container_vector, basic)
TESTC(container_vector, reserve)
TESTC(container_vector, move_only)
TESTC(container_vector, to)
TESTC(container_vector, clone)
TESTC(container_vector, compare)
TESTC(container_vector, static_)
}
