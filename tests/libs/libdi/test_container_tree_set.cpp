#include <di/prelude.h>
#include <test/test.h>

class RNG {
public:
    constexpr explicit RNG(u32 seed) : m_seed(seed) {}

    constexpr u32 next() {
        m_seed = m_seed * 1103515245 + 12345;
        m_seed %= 32767;
        return m_seed;
    }

private:
    u32 m_seed;
};

constexpr void basic() {
    di::TreeSet<int> x;
    x.clear();
    ASSERT_EQ(di::distance(x), 0);

    x.insert_container(di::Array { 1, 2, 5, 0, 4, -6, 6, 3 });

    auto [it, did_insert] = x.insert(3);
    ASSERT_EQ(*it, 3);
    ASSERT(!did_insert);

    ASSERT_EQ(di::distance(x), 8);
    ASSERT_EQ(di::sum(x), 15);

    ASSERT(di::is_sorted(x));
}

constexpr void accessors() {
    auto x = di::range(1, 6) | di::to<di::TreeSet>(di::compare);

    auto const& y = x;
    ASSERT_EQ(*y.lower_bound(3), 3);
    ASSERT_EQ(*y.upper_bound(3), 4);
    ASSERT_EQ(*y.find(3), 3);
    ASSERT_EQ(*y.at(3), 3);
    ASSERT_EQ(y.at(6), di::nullopt);
    ASSERT_EQ(*y.front(), 1);
    ASSERT_EQ(*y.back(), 5);
    ASSERT_EQ(y.count(3), 1u);
    ASSERT(y.lower_bound(10) == y.end());
    ASSERT(y.upper_bound(10) == y.end());
    ASSERT_EQ(y.count(10), 0u);
}

constexpr void erase() {
    auto x = di::to<di::TreeSet>(di::range(1, 6));

    ASSERT_EQ(*x.erase(x.find(2)), 3);
    ASSERT_EQ(*x.erase(x.find(3), x.find(5)), 5);
    ASSERT_EQ(x.erase(1), 1u);
    ASSERT_EQ(x.erase(6), 0u);
}

constexpr void property() {
    auto do_test = [](auto rng) {
        auto x = di::TreeSet<unsigned int> {};

        auto iterations = di::is_constant_evaluated() ? 99 : 345;
        for (auto i : di::range(iterations)) {
            x.insert(rng.next());
            ASSERT(di::is_sorted(x));
            ASSERT(di::is_sorted(di::reverse(x), di::compare_backwards));
            ASSERT_EQ(di::distance(x.begin(), x.end()), i + 1);
            ASSERT_EQ(di::distance(di::reverse(x).begin(), di::reverse(x).end()), i + 1);
            ASSERT_EQ(di::size(x), di::to_unsigned(i) + 1);
        }
    };

    do_test(RNG(1));

    if (!di::is_constant_evaluated()) {
        do_test(RNG(2));
        do_test(RNG(3));
        do_test(RNG(4));
        do_test(RNG(5));
        do_test(RNG(6));
        do_test(RNG(7));
        do_test(RNG(8));
        do_test(RNG(9));
        do_test(RNG(10));
    }
}

constexpr void xx() {
    auto x = di::Array { U'C', U'D', U'D', U'F', U'J', U'L', U'N', U'S', U'V', U'b', U'd',
                         U'g', U'g', U'j', U'j', U'm', U'p', U's', U't', U'v', U'w' } |
             di::to<di::TreeSet>();
    auto y = di::Array { U'B', U'N', U'Q', U'W', U'Z', U'b', U'c', U'f', U'j', U's', U's', U't', U'w' } | di::to<di::TreeSet>();

    auto z = di::Array { U'B', U'C', U'D', U'F', U'F', U'G', U'H', U'M', U'M', U'N', U'Q',
                         U'Q', U'R', U'V', U'V', U'W', U'f', U'l', U'p', U'p', U'r' } |
             di::to<di::TreeSet>();

    x &= y;
    x &= z;
}

TEST_CONSTEXPR(container_tree_set, basic, basic)
TEST_CONSTEXPR(container_tree_set, accessors, accessors)
TEST_CONSTEXPR(container_tree_set, erase, erase)
TEST_CONSTEXPR(container_tree_set, property, property)
TEST_CONSTEXPR(container_tree_set, xx, xx)