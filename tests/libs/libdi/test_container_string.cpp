#include <di/concepts/always_true.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = "QWER"_sv;
    (void) x;

    ASSERT_EQ(x.size(), 4u);

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
    ASSERT_EQ(x.size(), 3u);
    ASSERT_EQ(x, "abc"_sv);

    x.append("def"_sv);
    ASSERT_EQ(x, "abcdef"_sv);
}

constexpr void to() {
    auto x = "abc"_sv | di::to<di::String>();
    ASSERT_EQ(x, "abc"_sv);

    auto y = "abc"_sv | di::to<di::String>(di::encoding::assume_valid);
    ASSERT_EQ(y, "abc"_sv);
}

TEST_CONSTEXPR(container_string, basic, basic)
TEST_CONSTEXPR(container_string, push_back, push_back)
TEST_CONSTEXPR(container_string, to, to)
