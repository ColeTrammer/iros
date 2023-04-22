#include <di/container/algorithm/prelude.h>
#include <di/container/hash/node/prelude.h>
#include <di/util/prelude.h>
#include <dius/test/prelude.h>

namespace container_node_hash_map {
constexpr void basic() {
    auto x = di::NodeHashMap<int, int> {};
    x.reserve(10);

    x.insert({ 1, 1 });
    x.insert({ 2, 2 });
    x.insert({ 3, 3 });
    x.insert({ 4, 4 });
    x.insert({ 5, 5 });

    ASSERT_EQ(x.size(), 5);

    auto ex1 = di::Array { di::make_tuple(1, 1), di::make_tuple(2, 2), di::make_tuple(3, 3), di::make_tuple(4, 4),
                           di::make_tuple(5, 5) } |
               di::to<di::Vector>();
    auto r1 = x | di::to<di::Vector>();
    di::sort(r1);
    ASSERT_EQ(r1, ex1);

    x.insert({ 1, 1 });
    ASSERT_EQ(x.size(), 5);

    x.erase(1);
    ASSERT_EQ(x.size(), 4);

    x.erase(1);
    ASSERT_EQ(x.size(), 4);

    x.erase(2);
    ASSERT_EQ(x.size(), 3);

    auto ex2 = di::Array { di::make_tuple(3, 3), di::make_tuple(4, 4), di::make_tuple(5, 5) } | di::to<di::Vector>();
    auto r2 = x | di::to<di::Vector>();
    di::sort(r2);
    ASSERT_EQ(r2, ex2);

    ASSERT_EQ(x[3], 3);

    x[6] = 1;
    ASSERT_EQ(x[6], 1);
}

constexpr void multi() {
    auto x = di::NodeHashMultiMap<int, int> {};
    x.insert({ 1, 1 });
    x.insert({ 1, 2 });
    x.insert({ 1, 3 });

    ASSERT_EQ(x.size(), 3);

    auto ex1 = di::Array { di::make_tuple(1, 1), di::make_tuple(1, 2), di::make_tuple(1, 3) } | di::to<di::Vector>();
    auto r1 = x | di::to<di::Vector>();
    di::sort(r1);
    ASSERT_EQ(r1, ex1);
}

constexpr void stress() {
    auto const iters = di::is_constant_evaluated() ? 100 : 10000;

    auto x = di::NodeHashMap<int, int> {};
    for (auto i = 0; i < iters; ++i) {
        auto old_size = x.size();
        auto [it, b] = x.insert({ i, i });
        ASSERT_EQ(*it, di::make_tuple(i, i));
        ASSERT(b);
        ASSERT_EQ(x.size(), old_size + 1);
    }

    ASSERT_EQ(x.size(), iters);
    for (auto i = 0; i < iters; ++i) {
        ASSERT_EQ(x.at(i), i);
    }

    auto r1 = x | di::to<di::Vector>();
    auto ex1 = di::range(iters) | di::transform([](auto i) {
                   return di::make_tuple(i, i);
               }) |
               di::to<di::Vector>();
    di::sort(r1);
    ASSERT_EQ(r1, ex1);
}

// NOTE: GCC refuses to compile anything involving linked-lists at compile-time.
TESTC_CLANG(container_node_hash_map, basic)
TESTC_CLANG(container_node_hash_map, multi)
TESTC_CLANG(container_node_hash_map, stress)
}
