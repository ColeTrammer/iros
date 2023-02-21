#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void basic() {
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
}

TESTC(container_tree_map, basic)
