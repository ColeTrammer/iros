#include <di/concepts/always_true.h>
#include <di/prelude.h>
#include <dius/test/prelude.h>

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

    auto y = x.to_owned();
    ASSERT_EQ(x, y);
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

    auto y = di::container::string::StringImpl<di::container::string::Utf8Encoding,
                                               di::StaticVector<c8, di::meta::SizeConstant<64>>> {};
    (void) y.push_back(U'a');

    ASSERT_EQ(y, "a"_sv);
}

constexpr void to() {
    auto x = u8"abc"_sv | di::to<di::String>();
    ASSERT_EQ(x, u8"abc"_sv);

    auto y = "abc"_sv | di::to<di::String>(di::encoding::assume_valid);
    ASSERT_EQ(y, "abc"_sv);
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

    ASSERT(x.starts_with(u8"$"_sv));
    ASSERT(x.ends_with(u8"€𐍈"_sv));
    ASSERT(!x.ends_with(u8"¢"_sv));

    ASSERT_EQ(x.front(), U'$');
    ASSERT_EQ(x.back(), U'𐍈');

    ASSERT_EQ(x.size_bytes(), 10u);
    ASSERT_EQ(di::distance(x), 4);

    ASSERT(u8"¢€"_sv == x.substr(*x.iterator_at_offset(1), *x.iterator_at_offset(6)));
    ASSERT(!x.iterator_at_offset(2));
    ASSERT(!x.iterator_at_offset(4));

    ASSERT(x.substr(x.begin(), *x.iterator_at_offset(1)) == u8"$"_sv);
    ASSERT(x.substr(*x.iterator_at_offset(1)) == u8"¢€𐍈"_sv);

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

constexpr void readonly_api() {
    auto s = u8"Hello, 世界, Hello 友達!"_sv;

    ASSERT(s.starts_with("Hel"_sv));
    ASSERT(s.starts_with(U'H'));

    ASSERT(s.ends_with(u8"友達!"_sv));
    ASSERT(s.ends_with(U'!'));

    ASSERT(s.contains(u8"世界"_sv));

    ASSERT_EQ(s.find("Hello"_sv).begin(), s.iterator_at_offset(0));
    ASSERT_EQ(s.find(U'界').begin(), s.iterator_at_offset(u8"Hello, 世"_sv.size_code_units()));

    ASSERT_EQ(s.rfind("llo"_sv).begin(), s.iterator_at_offset(u8"Hello, 世界, He"_sv.size_code_units()));
    ASSERT_EQ(s.rfind("llo"_sv).end(), s.iterator_at_offset(u8"Hello, 世界, Hello"_sv.size_code_units()));
    ASSERT_EQ(s.rfind(U'l').begin(), s.iterator_at_offset(u8"Hello, 世界, Hel"_sv.size_code_units()));

    ASSERT_EQ(*s.find_first_of(u8"o達"_sv), U'o');
    ASSERT_EQ(*s.find_last_of(u8"o達"_sv), U'達');

    ASSERT_EQ(*s.find_first_not_of(u8"o達!"_sv), U'H');
    ASSERT_EQ(*s.find_last_not_of(u8"o達!"_sv), U'友');
}

constexpr void null_terminated() {
    auto s = di::TransparentString {};

    s.push_back('a');
    ASSERT_EQ(di::distance(di::ZCString { s.c_str() }), 1);

    s.push_back('b');
    ASSERT_EQ(di::distance(di::ZCString { s.c_str() }), 2);
}

TESTC(container_string, basic)
TESTC(container_string, push_back)
TESTC(container_string, to)
TESTC(container_string, erased)
TESTC(container_string, utf8)
TESTC(container_string, readonly_api)
TESTC(container_string, null_terminated)
