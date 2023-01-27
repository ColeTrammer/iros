#include <dius/test/prelude.h>
#include <string.h>

[[gnu::noinline]] static char const* do_strchr(char const* s, int ch) {
    return strchr(s, ch);
}

static void strchr_() {
    auto s = di::black_box((char const*) "Hello");

    auto r1 = do_strchr(s, 'l');
    auto e1 = s + 2;
    ASSERT_EQ(r1, e1);

    auto r2 = do_strchr(s, '\0');
    auto e2 = s + 5;
    ASSERT_EQ(r2, e2);

    auto r3 = do_strchr(s, 'p');
    auto e3 = nullptr;
    ASSERT_EQ(r3, e3);

    auto t = di::black_box((char const*) "");
    auto r4 = do_strchr(t, 'a');
    auto e4 = nullptr;
    ASSERT_EQ(r4, e4);

    auto r5 = do_strchr(t, '\0');
    auto e5 = t;
    ASSERT_EQ(r5, e5);
}

DIUS_TEST(string_h, strchr_)