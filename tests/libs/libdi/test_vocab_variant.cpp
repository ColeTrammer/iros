#include <di/function/index_dispatch.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto v = di::Variant<int, short, long>(di::in_place_index<1>, 1);
    auto w = di::Variant<int, short, long>();

    auto s = di::get<1>(v);
    EXPECT_EQ(s, 1);

    auto x = di::get<0>(w);
    EXPECT_EQ(x, 0);

    static_assert(di::vocab::detail::MemberVariantIndex<decltype(w)>);
    static_assert(di::concepts::VariantLike<decltype(w)>);

    EXPECT(di::holds_alternative<short>(v));
    EXPECT(di::holds_alternative<int>(w));

    EXPECT(di::get<int>(w) == 0);

    EXPECT(di::get_if<short>(w) == di::nullopt);
    EXPECT(di::get_if<int>(w) == 0);
    EXPECT(di::get_if<int>(di::move(w)) == 0);

    EXPECT(di::visit<int>(
               [](auto x) {
                   return x;
               },
               v) == 1);

    static_assert(di::SameAs<unsigned char, di::math::SmallestUnsignedType<6>>);
    static_assert(di::SameAs<unsigned short, di::math::SmallestUnsignedType<256>>);
}

TEST_CONSTEXPR(vocab_variant, basic, basic)
