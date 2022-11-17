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

    x.insert(1);
    x.insert(2);
    x.insert(5);
    x.insert(0);
    x.insert(4);
    x.insert(-6);
    x.insert(6);
    x.insert(3);

    ASSERT_EQ(di::distance(x), 8);
    ASSERT_EQ(di::sum(x), 15);

    ASSERT(di::is_sorted(x));
}

constexpr void accessors() {
    auto x = di::TreeSet<int> {};

    for (auto i : di::range(1, 6)) {
        x.insert(i);
    }

    ASSERT_EQ(*x.lower_bound(3), 3);
    ASSERT_EQ(*x.upper_bound(3), 4);
    ASSERT_EQ(*x.find(3), 3);
    ASSERT_EQ(*x.at(3), 3);
    ASSERT_EQ(x.at(6), di::nullopt);
    ASSERT_EQ(*x.front(), 1);
    ASSERT_EQ(*x.back(), 5);
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

TEST_CONSTEXPR(container_tree_set, basic, basic)
TEST_CONSTEXPR(container_tree_set, accessors, accessors)
TEST_CONSTEXPR(container_tree_set, property, property)