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

[[gnu::noinline]] static void* do_memcpy(di::Byte* dest, di::Byte const* src, size_t n) {
    return memcpy(dest, src, di::black_box(n));
}

[[gnu::noinline]] static void* do_memmove(di::Byte* dest, di::Byte const* src, size_t n) {
    return memmove(dest, src, di::black_box(n));
}

[[gnu::noinline]] static void* do_memset(di::Byte* dest, di::Byte x, size_t n) {
    return memset(dest, di::to_integer<int>(x), di::black_box(n));
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

static void memcpy_() {
    auto src = di::black_box(di::Array { di::Byte(4), di::Byte(5), di::Byte(6), di::Byte(7) });
    auto dst = di::black_box(di::Array { di::Byte(8), di::Byte(9), di::Byte(10), di::Byte(11) });

    ASSERT_EQ(dst.data() + 1, do_memcpy(dst.data() + 1, src.data(), 2));

    auto e1 = di::Array { di::Byte(8), di::Byte(4), di::Byte(5), di::Byte(11) };
    ASSERT_EQ(dst, e1);
}

static void memmove_() {
    auto bytes = di::black_box(di::Array { di::Byte(4), di::Byte(5), di::Byte(6), di::Byte(7), di::Byte(8) });
    ASSERT_EQ(bytes.data(), do_memmove(bytes.data(), bytes.data() + 1, 2));

    auto e1 = di::Array { di::Byte(5), di::Byte(6), di::Byte(6), di::Byte(7), di::Byte(8) };
    ASSERT_EQ(bytes, e1);

    ASSERT_EQ(bytes.data() + 3, do_memmove(bytes.data() + 3, bytes.data() + 2, 2));

    auto e2 = di::Array { di::Byte(5), di::Byte(6), di::Byte(6), di::Byte(6), di::Byte(7) };
    ASSERT_EQ(bytes, e2);
}

static void memset_() {
    auto bytes = di::black_box(di::Array { di::Byte(4), di::Byte(5), di::Byte(6), di::Byte(7) });
    ASSERT_EQ(bytes.data(), do_memset(bytes.data(), di::Byte(9), 3));

    auto ex1 = di::Array { di::Byte(9), di::Byte(9), di::Byte(9), di::Byte(7) };
    ASSERT_EQ(bytes, ex1);
}

TEST(string_h, strchr_)
TEST(string_h, strstr_)
TEST(string_h, strlen_)
TEST(string_h, memcpy_)
TEST(string_h, memmove_)
TEST(string_h, memset_)