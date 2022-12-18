#include <di/concepts/always_true.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = u8"QWER"_sv;
    (void) x;

    ASSERT_EQ(x.size_code_units(), 4u);

    int c = 0;
    for (auto xx : x) {
        (void) xx;
        c++;
    }
    ASSERT_EQ(c, 4);
}

constexpr void push_back() {
    auto x = di::String {};
    x.push_back('a');
    x.push_back('b');
    x.push_back('c');
    ASSERT_EQ(x.size_code_units(), 3u);
    ASSERT_EQ(x, u8"abc"_sv);

    x.append(u8"def"_sv);
    ASSERT_EQ(x, u8"abcdef"_sv);
}

constexpr void to() {
    auto x = u8"abc"_sv | di::to<di::String>();
    ASSERT_EQ(x, u8"abc"_sv);

    auto y = u8"abc"_sv | di::to<di::String>(di::encoding::assume_valid);
    ASSERT_EQ(y, u8"abc"_sv);
}

constexpr void erased() {
    di::ErasedString s = u8"abc"_sv;
    ASSERT_EQ(s, u8"abc"_sv);
}

constexpr void do_utf8_test(di::StringView view, di::Vector<char32_t> const& desired) {
    // Check the iteration produces the same results forwards and backwards.
    auto forwards = view | di::to<di::Vector>();
    auto backwards = view | di::reverse | di::to<di::Vector>();
    di::container::reverse(backwards);

    ASSERT_EQ(forwards, backwards);
    ASSERT_EQ(forwards, desired);
    ASSERT_EQ(backwards, desired);
};

constexpr void utf8() {
    auto x = u8"$¢€𐍈"_sv;

    // ASSERT(x.starts_with(u8"$"_sv));
    // ASSERT(x.ends_with(u8"€𐍈"_sv));
    // ASSERT(!x.ends_with(u8"¢"_sv));

    // ASSERT_EQ(x.front(), U'$');
    // ASSERT_EQ(x.back(), U'𐍈');

    ASSERT_EQ(x.size_bytes(), 10u);
    ASSERT_EQ(di::distance(x), 4);

    // ASSERT("¢€"_sv == x.substr(*x.iterator_at_offset(1), *x.iterator_at_offset(6)));
    // ASSERT(!x.iterator_at_offset(2));
    // ASSERT(!x.iterator_at_offset(4));

    // ASSERT(x.substr(x.begin(), *x.iterator_at_offset(1)) == u8"$"_sv);
    // ASSERT(x.substr(*x.iterator_at_offset(1)) == u8"¢€𐍈"_sv);

    auto s = di::String {};
    s.push_back(U'$');
    s.push_back(U'¢');
    s.push_back(U'€');
    s.push_back(U'𐍈');

    ASSERT_EQ(s, u8"$¢€𐍈"_sv);
    do_utf8_test(s.view(), di::Array {
                               U'\x24',
                               U'\xA2',
                               U'\x20AC',
                               U'\x10348',
                           } | di::to<di::Vector>());

    auto validate = [](c8 const* x, size_t n) {
        auto span = di::Span(x, n);
        auto encoding = di::container::string::Utf8Encoding {};
        return di::encoding::validate(encoding, span);
    };

    ASSERT(!validate(u8"\xFF", 1));
    ASSERT(!validate(u8"\xC0\xAF", 2));
    ASSERT(!validate(u8"\xE0\x0F\x80", 3));
    ASSERT(!validate(u8"\xF0\x82\x82\xAC", 4));
}

TEST_CONSTEXPR(container_string, basic, basic)
TEST_CONSTEXPR(container_string, push_back, push_back)
TEST_CONSTEXPR(container_string, to, to)
TEST_CONSTEXPR(container_string, erased, erased)
TEST_CONSTEXPR(container_string, utf8, utf8)