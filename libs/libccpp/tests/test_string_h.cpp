#include <dius/test/prelude.h>
#include <string.h>

[[gnu::noinline]] static char const* do_strchr(char const* s, int ch) {
    return strchr(s, ch);
}

[[gnu::noinline]] static char const* do_strstr(char const* s, char const* t) {
    return strstr(s, t);
}

[[gnu::noinline]] static size_t do_strlen(char const* s) {
    return strlen(s);
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

static void strstr_() {
    auto s = di::black_box((char const*) "Hello");
    auto t = di::black_box((char const*) "ell");
    auto v = di::black_box((char const*) "lll");
    auto u = di::black_box((char const*) "Helloo");
    auto e = di::black_box((char const*) "");

    auto r1 = do_strstr(s, t);
    auto e1 = s + 1;
    ASSERT_EQ(r1, e1);

    auto r2 = do_strstr(s, v);
    auto e2 = nullptr;
    ASSERT_EQ(r2, e2);

    auto r3 = do_strstr(s, u);
    auto e3 = nullptr;
    ASSERT_EQ(r3, e3);

    auto r4 = do_strstr(s, e);
    auto e4 = s;
    ASSERT_EQ(r4, e4);

    auto r5 = do_strstr(e, e);
    auto e5 = e;
    ASSERT_EQ(r5, e5);
}

static void strlen_() {
    auto s = di::black_box((char const*) "Hello");
    auto e = di::black_box((char const*) "");

    auto r1 = do_strlen(s);
    auto e1 = 5u;
    ASSERT_EQ(r1, e1);

    auto r2 = do_strlen(e);
    auto e2 = 0u;
    ASSERT_EQ(r2, e2);
}

TEST(string_h, strchr_)
TEST(string_h, strstr_)
TEST(string_h, strlen_)