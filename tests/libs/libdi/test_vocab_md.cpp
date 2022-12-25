#include <di/prelude.h>
#include <test/test.h>

constexpr void extents() {
    static_assert(di::SameAs<di::Dextents<size_t, 5>, di::Extents<size_t, di::dynamic_extent, di::dynamic_extent, di::dynamic_extent,
                                                                  di::dynamic_extent, di::dynamic_extent>>);

    auto a = di::Extents { 1, 2, 3, 4, 5 };
    ASSERT_EQ(a.extent(0), 1u);
    ASSERT_EQ(a.extent(1), 2u);
    ASSERT_EQ(a.extent(2), 3u);
    ASSERT_EQ(a.extent(3), 4u);
    ASSERT_EQ(a.extent(4), 5u);

    auto b = di::Extents<size_t, di::dynamic_extent, 3, di::dynamic_extent, 5, di::dynamic_extent> { 2, 4, 6 };
    ASSERT_EQ(b.extent(0), 2u);
    ASSERT_EQ(b.extent(1), 3u);
    ASSERT_EQ(b.extent(2), 4u);
    ASSERT_EQ(b.extent(3), 5u);
    ASSERT_EQ(b.extent(4), 6u);

    auto c =
        di::Extents<size_t, di::dynamic_extent, 3, di::dynamic_extent, 5, di::dynamic_extent> { di::to_array<size_t>({ 2, 3, 4, 5, 6 }) };
    ASSERT_EQ(c.extent(0), 2u);
    ASSERT_EQ(c.extent(1), 3u);
    ASSERT_EQ(c.extent(2), 4u);
    ASSERT_EQ(c.extent(3), 5u);
    ASSERT_EQ(c.extent(4), 6u);

    ASSERT_EQ(b, c);
    ASSERT_NOT_EQ(a, c);

    ASSERT_EQ(c.fwd_prod_of_extents(0), 1u);
    ASSERT_EQ(c.fwd_prod_of_extents(2), 6u);
    ASSERT_EQ(c.rev_prod_of_extents(2), 30u);
    ASSERT_EQ(c.rev_prod_of_extents(4), 1u);
}

constexpr void layout_left() {
    di::concepts::MDLayoutMapping auto mapping = di::LayoutLeft::Mapping<di::Extents<size_t, 4, 2, 3>> {};

    ASSERT_EQ(mapping.required_span_size(), 4u * 2u * 3u);

    ASSERT_EQ(mapping.stride(0), 1u);
    ASSERT_EQ(mapping.stride(1), 4u);
    ASSERT_EQ(mapping.stride(2), 8u);
    ASSERT_EQ(mapping(1, 1, 1), 1u + 4u + 8u);
}

constexpr void layout_right() {
    di::concepts::MDLayoutMapping auto mapping = di::LayoutRight::Mapping<di::Extents<size_t, 4, 2, 3>> {};

    ASSERT_EQ(mapping.required_span_size(), 4u * 2u * 3u);

    ASSERT_EQ(mapping.stride(0), 6u);
    ASSERT_EQ(mapping.stride(1), 3u);
    ASSERT_EQ(mapping.stride(2), 1u);
    ASSERT_EQ(mapping(1, 1, 1), 6u + 3u + 1u);
}

constexpr void default_accessor() {
    di::concepts::MDAccessor auto accessor = di::DefaultAccessor<int> {};

    int p[] = { 1, 2, 3, 4, 5 };
    ASSERT_EQ(accessor.access(p, 2), 3);
    ASSERT_EQ(accessor.offset(p, 2), p + 2);
}

constexpr void mdspan() {
    auto storage = di::Array { 1, 2, 3, 4, 5, 6, 7, 8 };
    auto span = di::MDSpan { storage.data(), di::Extents<size_t, 2, 2, 2> {} };

    ASSERT_EQ((span[0, 0, 0]), 1);
    ASSERT_EQ((span[0, 0, 1]), 2);
    ASSERT_EQ((span[0, 1, 0]), 3);
    ASSERT_EQ((span[0, 1, 1]), 4);
    ASSERT_EQ((span[1, 0, 0]), 5);
    ASSERT_EQ((span[1, 0, 1]), 6);
    ASSERT_EQ((span[1, 1, 0]), 7);
    ASSERT_EQ((span[1, 1, 1]), 8);
    ASSERT_EQ(span.size(), 8u);
    ASSERT(!span.empty());

    auto lspan = di::MDSpan { storage.data(), di::LayoutLeft::Mapping(di::Extents<size_t, 2, 2, 2> {}) };

    ASSERT_EQ((lspan[0, 0, 0]), 1);
    ASSERT_EQ((lspan[0, 0, 1]), 5);
    ASSERT_EQ((lspan[0, 1, 0]), 3);
    ASSERT_EQ((lspan[0, 1, 1]), 7);
    ASSERT_EQ((lspan[1, 0, 0]), 2);
    ASSERT_EQ((lspan[1, 0, 1]), 6);
    ASSERT_EQ((lspan[1, 1, 0]), 4);
    ASSERT_EQ((lspan[1, 1, 1]), 8);
    ASSERT_EQ(lspan.size(), 8u);
    ASSERT(!lspan.empty());
}

TEST_CONSTEXPR(vocab_md, extents, extents)
TEST_CONSTEXPR(vocab_md, layout_left, layout_left)
TEST_CONSTEXPR(vocab_md, layout_right, layout_right)
TEST_CONSTEXPR(vocab_md, default_accessor, default_accessor)
TEST_CONSTEXPR(vocab_md, mdspan, mdspan)