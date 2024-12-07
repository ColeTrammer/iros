#include "di/container/algorithm/prelude.h"
#include "di/container/linked/prelude.h"
#include "di/container/view/prelude.h"
#include "dius/test/prelude.h"

namespace container_linked_list {
constexpr static void basic() {
    auto x = di::LinkedList<int> {};

    x.push_back(1);
    x.push_back(2);
    x.push_back(3);
    ASSERT_EQ(di::sum(x), 6);
    ASSERT_EQ(di::distance(x.begin(), x.end()), 3);

    ASSERT_EQ(x.size(), 3U);
    ASSERT_EQ(x.pop_back(), 3);
    ASSERT_EQ(x.pop_back(), 2);
    ASSERT_EQ(x.pop_back(), 1);

    ASSERT(x.empty());
    ASSERT_EQ(di::sum(x), 0);
    ASSERT_EQ(di::distance(x.begin(), x.end()), 0);

    auto y = di::range(5) | di::to<di::LinkedList>();
    ASSERT(di::container::equal(y, di::range(5)));

    auto z = di::range(5) | di::to<di::LinkedList>();
    ASSERT_EQ(y, z);

    auto a = di::LinkedList<int> {};
    a.prepend_container(di::range(3));
    a.append_container(di::range(6, 9));
    a.insert_container(di::next(a.begin(), 3), di::range(3, 6));

    auto b = di::range(9) | di::to<di::LinkedList>();
    ASSERT_EQ(a, b);
}

constexpr static void splice() {
    auto a = di::range(5) | di::to<di::LinkedList>();
    auto b = di::range(8, 17) | di::to<di::LinkedList>();

    a.splice(a.begin(), b, b.begin());
    auto ex1 = di::Array { 8, 0, 1, 2, 3, 4 } | di::to<di::LinkedList>();

    ASSERT_EQ(a.size(), 6U);
    ASSERT_EQ(b.size(), 8U);
    ASSERT_EQ(a, ex1);

    a.splice(a.end(), b, di::prev(b.end(), 4), b.end());
    auto ex2 = di::Array { 8, 0, 1, 2, 3, 4, 13, 14, 15, 16 } | di::to<di::LinkedList>();

    ASSERT_EQ(a.size(), 10U);
    ASSERT_EQ(b.size(), 4U);
    ASSERT_EQ(a, ex2);

    a.splice(di::next(a.begin(), 2), b);
    auto ex3 = di::Array { 8, 0, 9, 10, 11, 12, 1, 2, 3, 4, 13, 14, 15, 16 } | di::to<di::LinkedList>();

    ASSERT_EQ(a.size(), 14U);
    ASSERT_EQ(b.size(), 0U);
    ASSERT_EQ(a, ex3);
}

TESTC_CLANG(container_linked_list, basic)
TESTC_CLANG(container_linked_list, splice)
}
