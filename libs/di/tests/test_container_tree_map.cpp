#include <di/container/interface/erase.h>
#include <di/container/tree/prelude.h>
#include <di/container/view/prelude.h>
#include <dius/test/prelude.h>

namespace container_tree_map {
constexpr static void basic() {
    auto x = di::zip(di::range(4), di::range(4)) | di::to<di::TreeMap>();

    ASSERT_EQ(x.at(1), 1);
    ASSERT_EQ(x.at(2), 2);
    ASSERT_EQ(x.at(3), 3);

    auto [it, did_insert] = x.insert_or_assign(2, 5);
    ASSERT_EQ(*it, (di::Tuple { 2, 5 }));
    ASSERT(!did_insert);

    auto [jt, jid_insert] = x.try_emplace(4, 8);
    ASSERT_EQ(*jt, di::make_tuple(4, 8));
    ASSERT(jid_insert);

    x[5] = 5;
    ASSERT_EQ(x.at(5), 5);

    auto y = di::TreeMap<di::String, di::String> {};
    y.try_emplace("hello"_sv, "world"_sv);
    ASSERT_EQ(y.at("hello"_sv), "world"_sv);

    auto z = di::Array { di::Tuple { 1, 1 }, di::Tuple { 2, 2 } } | di::to<di::TreeMap>();
    ASSERT_EQ(z.at(1), 1);
    ASSERT_EQ(z.at(2), 2);

    ASSERT_EQ(di::erase_if(z,
                           [](auto x) {
                               return di::get<0>(x) == 1;
                           }),
              1U);
    ASSERT_EQ(z.at(2), 2);
    ASSERT_EQ(z.size(), 1U);
}

TESTC(container_tree_map, basic)
}
