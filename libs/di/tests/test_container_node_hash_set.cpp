#include "di/container/algorithm/prelude.h"
#include "di/container/hash/node/prelude.h"
#include "di/container/interface/erase.h"
#include "dius/test/prelude.h"

namespace container_node_hash_set {
constexpr static void basic() {
    auto x = di::NodeHashSet<int> {};
    x.reserve(10);

    x.insert(1);
    x.insert(2);
    x.insert(3);
    x.insert(4);
    x.insert(5);

    ASSERT_EQ(x.size(), 5);

    auto ex1 = di::Array { 1, 2, 3, 4, 5 } | di::to<di::Vector>();
    auto r1 = x | di::to<di::Vector>();
    di::sort(r1);
    ASSERT_EQ(r1, ex1);

    x.insert(1);
    ASSERT_EQ(x.size(), 5);

    x.erase(1);
    ASSERT_EQ(x.size(), 4);

    x.erase(1);
    ASSERT_EQ(x.size(), 4);

    x.erase(2);
    ASSERT_EQ(x.size(), 3);

    auto ex2 = di::Array { 3, 4, 5 } | di::to<di::Vector>();
    auto r2 = x | di::to<di::Vector>();
    di::sort(r2);
    ASSERT_EQ(r2, ex2);

    ASSERT_EQ(di::erase_if(x,
                           [](auto x) {
                               return x == 3;
                           }),
              1U);
    ASSERT_EQ(x.size(), 2);
}

constexpr static void multi() {
    auto x = di::NodeHashMultiSet<int> {};
    x.insert(1);
    x.insert(1);
    x.insert(1);

    ASSERT_EQ(x.size(), 3);

    auto ex1 = di::Array { 1, 1, 1 } | di::to<di::Vector>();
    auto r1 = x | di::to<di::Vector>();
    di::sort(r1);
    ASSERT_EQ(r1, ex1);
}

// NOTE: GCC refuses to compile anything involving linked-lists at compile-time.
TESTC_CLANG(container_node_hash_set, basic)
TESTC_CLANG(container_node_hash_set, multi)
}
