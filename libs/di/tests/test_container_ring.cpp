#include <di/container/ring/prelude.h>
#include <dius/test/prelude.h>

namespace container_ring {
constexpr void basic() {
    di::concepts::RandomAccessContainer auto v = di::Ring<int> {};
    ASSERT(v.empty());
    ASSERT(!v.front());
    ASSERT(!v.back());

    v.push_back(5);
    ASSERT_EQ(v.size(), 1U);
    ASSERT_EQ(v[0], 5);
    ASSERT_EQ(v.front(), 5);
    ASSERT_EQ(v.back(), 5);

    v.push_back(6);
    ASSERT_EQ(v.size(), 2U);
    ASSERT_EQ(v[0], 5);
    ASSERT_EQ(v[1], 6);

    ASSERT_EQ(v.pop_back(), 6);
    ASSERT_EQ(v.pop_back(), 5);
    ASSERT(v.empty());

    v.push_front(5);
    ASSERT_EQ(v.size(), 1U);
    ASSERT_EQ(v[0], 5);
    ASSERT_EQ(v.front(), 5);
    ASSERT_EQ(v.back(), 5);

    v.push_front(6);
    ASSERT_EQ(v.size(), 2U);
    ASSERT_EQ(v[0], 6);
    ASSERT_EQ(v[1], 5);

    ASSERT_EQ(v.pop_front(), 6);
    ASSERT_EQ(v.pop_front(), 5);
    ASSERT(v.empty());

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    v.erase(v.iterator(2));
    ASSERT_EQ(v.size(), 4U);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 4);
    ASSERT_EQ(v[3], 5);

    v.erase(v.iterator(1), v.iterator(3));
    ASSERT_EQ(v.size(), 2U);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 5);

    v.insert(v.iterator(1), 3);
    ASSERT_EQ(v.size(), 3U);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 3);
    ASSERT_EQ(v[2], 5);

    v.resize(6);
    ASSERT_EQ(v.size(), 6U);
    ASSERT_EQ(v.back(), 0);

    v.resize(2);
    ASSERT_EQ(v.size(), 2U);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 3);

    v.clear();
    ASSERT(v.empty());
}

constexpr void reserve() {
    auto v = di::Ring<int> {};
    v.push_back(0);
    v.push_back(1);

    v.reserve(v.capacity() * 2);
    ASSERT_EQ(v, (di::Array { 0, 1 } | di::to<di::Ring>()));
}

struct M {
    constexpr M(int x_) : x(x_) {}

    constexpr M(M const&) = delete;
    constexpr M(M&& xx) : x(di::exchange(xx.x, 0)) {}

    constexpr ~M() {}

    int x;

    constexpr friend auto operator==(M const& a, M const& b) -> bool { return a.x == b.x; }
};

constexpr void move_only() {
    auto v = di::Ring<M> {};
    v.push_back(M { 2 });
    v.push_back(M { 4 });

    ASSERT_EQ(v[0].x, 2);
    ASSERT_EQ(v[1].x, 4);
    ASSERT_EQ(v.size(), 2U);

    auto w = di::move(v);
    ASSERT_EQ(w.size(), 2U);

    // NOLINTNEXTLINE(bugprone-use-after-move)
    ASSERT(v.empty());
}

constexpr void to() {
    auto v = di::create<di::Ring<int>>(di::range(6));
    ASSERT_EQ(v.size(), 6U);
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[4], 4);

    auto w = di::container::to<di::Ring<int>>(di::range(6));
    ASSERT_EQ(w.size(), 6U);
    ASSERT_EQ(w[0], 0);
    ASSERT_EQ(w[4], 4);

    auto x = di::create<di::Ring>(di::range(6));
    ASSERT_EQ(x.size(), 6U);
    ASSERT_EQ(x[0], 0);
    ASSERT_EQ(x[4], 4);

    auto y = di::container::to<di::Ring>(di::range(6));
    ASSERT_EQ(y.size(), 6U);
    ASSERT_EQ(y[0], 0);
    ASSERT_EQ(y[4], 4);

    auto u = di::range(6) | di::container::to<di::Ring<int>>();
    ASSERT_EQ(u.size(), 6U);
    ASSERT_EQ(u[0], 0);
    ASSERT_EQ(u[4], 4);

    auto z = di::range(6) | di::container::to<di::Ring>();
    ASSERT_EQ(z.size(), 6U);
    ASSERT_EQ(z[0], 0);
    ASSERT_EQ(z[4], 4);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Ring>(), di::range)) |
             di::container::to<di::Ring>();
    ASSERT_EQ(a.size(), 6U);
    ASSERT_EQ(a[0].size(), 0U);
    ASSERT_EQ(a[4].size(), 4U);
}

constexpr void clone() {
    auto v = di::range(6) | di::container::to<di::Ring>();
    ASSERT_EQ(v.size(), 6U);

    auto w = di::clone(v);
    ASSERT_EQ(w.size(), 6U);

    auto a = di::range(6) | di::transform(di::compose(di::container::to<di::Ring>(), di::range)) |
             di::container::to<di::Ring>();
    ASSERT_EQ(a.size(), 6U);
    ASSERT_EQ(a[0].size(), 0U);
    ASSERT_EQ(a[4].size(), 4U);

    auto b = di::clone(a);
    ASSERT_EQ(b.size(), 6U);
    ASSERT_EQ(b[0].size(), 0U);
    ASSERT_EQ(b[4].size(), 4U);
}

constexpr void compare() {
    auto a = di::create<di::Ring>(di::range(6));
    auto b = di::create<di::Ring>(di::range(4));
    auto c = di::create<di::Ring>(di::range(8, 100));
    ASSERT_NOT_EQ(a, b);
    ASSERT_EQ(a, a);
    ASSERT_GT(a, b);
    ASSERT_LT(a, c);
}

constexpr void static_() {
    auto a = di::StaticRing<int, di::Constexpr<2ZU>> {};
    (void) a.push_back(1);
    (void) a.push_back(2);
    (void) a.push_back(3);

    auto b = di::StaticRing<int, di::Constexpr<2ZU>> {};
    (void) b.push_back(1);
    (void) b.push_back(2);

    ASSERT_EQ(a, b);

    (void) a.resize(0);
    ASSERT_EQ(a.size(), 0U);

    (void) a.emplace(a.begin());
    (void) a.emplace(a.begin());
    (void) a.emplace(a.begin());
    ASSERT_EQ(a.size(), 2U);

    auto v = di::StaticRing<c8, di::Constexpr<4ZU>> {};
    (void) v.resize(1);
    v[0] = 9;

    ASSERT_EQ(v.size(), 1U);

    auto w = v;
    (void) w.append_container(di::move(v));

    ASSERT_EQ(w.size(), 2U);
}

TESTC(container_ring, basic)
TESTC(container_ring, reserve)
TESTC(container_ring, move_only)
TESTC(container_ring, to)
TESTC(container_ring, clone)
TESTC(container_ring, compare)
TESTC(container_ring, static_)
}
