#include <di/function/index_dispatch.h>
#include <di/vocab/variant/prelude.h>
#include <dius/test/prelude.h>

namespace vocab_variant {
constexpr void basic() {
    auto v = di::Variant<int, short, long>(di::c_<1ZU>, 1);
    auto w = di::Variant<int, short, long>();

    auto s = di::get<1>(v);
    ASSERT_EQ(s, 1);

    auto x = di::get<0>(w);
    ASSERT_EQ(x, 0);

    static_assert(di::vocab::detail::MemberVariantIndex<decltype(w)>);
    static_assert(di::concepts::VariantLike<decltype(w)>);

    ASSERT(di::holds_alternative<short>(v));
    ASSERT(di::holds_alternative<int>(w));

    ASSERT_EQ(di::get<int>(w), 0);

    ASSERT_EQ(di::get_if<short>(w), di::nullopt);
    ASSERT_EQ(di::get_if<int>(w), 0);
    ASSERT_EQ(di::get_if<int>(di::move(w)), 0);

    ASSERT_EQ(1, (di::visit<int>(
                     [](auto x) {
                         return x;
                     },
                     v)));

    ASSERT_EQ(1, (di::visit<int>(
                     [](auto x, auto y) {
                         return x + y;
                     },
                     v, w)));

    bool called = false;
    di::visit(
        [&](auto, auto) {
            called = true;
        },
        v, w);
    ASSERT(called);

    auto a = di::Variant<int, long>(di::in_place_type<int>, 42);
    auto b = di::Variant<int, long>(di::in_place_type<int>, 45);
    auto c = di::Variant<int, long>(di::in_place_type<int>, 42);
    auto d = di::Variant<int, long>(di::in_place_type<long>, 1L);
    ASSERT_NOT_EQ(a, b);
    ASSERT_LT(a, b);
    ASSERT_EQ(a, c);
    ASSERT_LT(a, d);

    auto e = di::Variant<int, double>(1);
    ASSERT_EQ(e.index(), 0U);
    ASSERT_EQ(e, 1);

    auto f = di::Variant<int, double>(1.5);
    ASSERT_EQ(f.index(), 1U);
    ASSERT_EQ(f, 1.5);

    f = 1;
    ASSERT_EQ(f.index(), 0U);
    ASSERT_EQ(f, 1);

    static_assert(di::SameAs<unsigned char, di::math::SmallestUnsignedType<6>>);
    static_assert(di::SameAs<unsigned short, di::math::SmallestUnsignedType<256>>);
}

TESTC(vocab_variant, basic)
}
