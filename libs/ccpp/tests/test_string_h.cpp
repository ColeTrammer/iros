#include <dius/test/prelude.h>
#include <string.h>

[[gnu::noinline]] static char const* do_strchr(char const* s, int ch) {
    return strchr(s, ch);
}

[[gnu::noinline]] static char const* do_strrchr(char const* s, int ch) {
    return strrchr(s, ch);
}

[[gnu::noinline]] static char const* do_strstr(char const* s, char const* t) {
    return strstr(s, t);
}

[[gnu::noinline]] static int do_strcmp(char const* s, char const* t) {
    return strcmp(s, t);
}

[[gnu::noinline]] static int do_strncmp(char const* s, char const* t, size_t count) {
    return strncmp(s, t, count);
}

[[gnu::noinline]] static char* do_strcat(char* s, char const* t) {
    return strcat(s, t);
}

[[gnu::noinline]] static char* do_strncat(char* s, char const* t, size_t count) {
    return strncat(s, t, count);
}

[[gnu::noinline]] static char* do_strcpy(char* s, char const* t) {
    return strcpy(s, t);
}

[[gnu::noinline]] static char* do_strncpy(char* s, char const* t, size_t count) {
    return strncpy(s, t, count);
}

[[gnu::noinline]] static usize do_strlen(char const* s) {
    return strlen(s);
}

[[gnu::noinline]] static int do_memcmp(byte* dest, byte const* src, usize n) {
    return memcmp(dest, src, di::black_box(n));
}

[[gnu::noinline]] static void* do_memcpy(byte* dest, byte const* src, usize n) {
    return memcpy(dest, src, di::black_box(n));
}

[[gnu::noinline]] static void* do_memmove(byte* dest, byte const* src, usize n) {
    return memmove(dest, src, di::black_box(n));
}

[[gnu::noinline]] static void* do_memset(byte* dest, byte x, usize n) {
    return memset(dest, di::to_integer<int>(x), di::black_box(n));
}

static void strcat_() {
    char buffer[16] = {};
    auto const* a = di::black_box((char const*) "Hello");
    auto const* b = di::black_box((char const*) ", World");

    ASSERT_EQ(buffer, do_strcat(do_strcpy(buffer, a), b));
    ASSERT_EQ(do_strcmp(buffer, "Hello, World"), 0);
}

static void strncat_() {
    char buffer[10] = {};
    auto const* a = di::black_box((char const*) "Hello");
    auto const* b = di::black_box((char const*) ", World");

    ASSERT_EQ(buffer, do_strncat(do_strcpy(buffer, a), b, sizeof(buffer) - 1));
    ASSERT_EQ(do_strcmp(buffer, "Hello, Wo"), 0);
}

static void strcmp_() {
    auto const* a = di::black_box((char const*) "Hello");
    auto const* b = di::black_box((char const*) "Hello");
    auto const* c = di::black_box((char const*) "HelloQ");
    auto const* d = di::black_box((char const*) "Hell");
    auto const* e = di::black_box((char const*) "Hellp");

    ASSERT_EQ(do_strcmp(a, b), 0);
    ASSERT_LT(do_strcmp(a, c), 0);
    ASSERT_GT(do_strcmp(a, d), 0);
    ASSERT_LT(do_strcmp(a, e), 0);
}

static void strncmp_() {
    auto const* a = di::black_box((char const*) "Hello");
    auto const* b = di::black_box((char const*) "Hello");
    auto const* c = di::black_box((char const*) "HelloQ");
    auto const* d = di::black_box((char const*) "Hell");
    auto const* e = di::black_box((char const*) "Hellp");

    ASSERT_EQ(do_strncmp(a, b, 5), 0);
    ASSERT_EQ(do_strncmp(a, b, 10), 0);
    ASSERT_EQ(do_strncmp(a, c, 5), 0);
    ASSERT_LT(do_strncmp(a, c, 6), 0);
    ASSERT_GT(do_strncmp(a, d, 5), 0);
    ASSERT_GT(do_strncmp(a, d, 10), 0);
    ASSERT_EQ(do_strncmp(a, d, 4), 0);
    ASSERT_LT(do_strncmp(a, e, 5), 0);
    ASSERT_LT(do_strncmp(a, e, 10), 0);
}

static void strcpy_() {
    char buffer[16] = {};
    di::fill(buffer, -1);
    auto const* s = di::black_box((char const*) "Hello");
    auto const* t = di::black_box((char const*) "");

    ASSERT_EQ(buffer, do_strcpy(buffer, s));
    ASSERT_EQ(do_strcmp(buffer, s), 0);

    ASSERT_EQ(buffer, do_strcpy(buffer, t));
    ASSERT_EQ(do_strcmp(buffer, t), 0);
}

static void strncpy_() {
    char buffer[8] = {};
    auto const* s = di::black_box((char const*) "Hello");
    auto const* t = di::black_box((char const*) "");
    auto const* q = di::black_box((char const*) "Hello, World");

    di::fill(buffer, -1);
    ASSERT_EQ(buffer, do_strncpy(buffer, s, 8));

    char expected1[] = { 'H', 'e', 'l', 'l', 'o', '\0', '\0', '\0' };
    ASSERT(di::container::equal(buffer, expected1));

    di::fill(buffer, -1);
    ASSERT_EQ(buffer, do_strncpy(buffer, t, 8));

    char expected2[] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
    ASSERT(di::container::equal(buffer, expected2));

    di::fill(buffer, -1);
    ASSERT_EQ(buffer, do_strncpy(buffer, q, 7));

    char expected3[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', -1 };
    ASSERT(di::container::equal(buffer, expected3));
}

static void strchr_() {
    auto const* s = di::black_box((char const*) "Hello");

    auto const* r1 = do_strchr(s, 'l');
    auto const* e1 = s + 2;
    ASSERT_EQ(r1, e1);

    auto const* r2 = do_strchr(s, '\0');
    auto const* e2 = s + 5;
    ASSERT_EQ(r2, e2);

    auto const* r3 = do_strchr(s, 'p');
    auto e3 = nullptr;
    ASSERT_EQ(r3, e3);

    auto const* t = di::black_box((char const*) "");
    auto const* r4 = do_strchr(t, 'a');
    auto e4 = nullptr;
    ASSERT_EQ(r4, e4);

    auto const* r5 = do_strchr(t, '\0');
    auto const* e5 = t;
    ASSERT_EQ(r5, e5);
}

static void strrchr_() {
    auto const* s = di::black_box((char const*) "Hello");

    auto const* r1 = do_strrchr(s, 'l');
    auto const* e1 = s + 3;
    ASSERT_EQ(r1, e1);

    auto const* r2 = do_strrchr(s, '\0');
    auto const* e2 = s + 5;
    ASSERT_EQ(r2, e2);

    auto const* r3 = do_strrchr(s, 'p');
    auto e3 = nullptr;
    ASSERT_EQ(r3, e3);

    auto const* t = di::black_box((char const*) "");
    auto const* r4 = do_strrchr(t, 'a');
    auto e4 = nullptr;
    ASSERT_EQ(r4, e4);

    auto const* r5 = do_strrchr(t, '\0');
    auto const* e5 = t;
    ASSERT_EQ(r5, e5);
}

static void strstr_() {
    auto const* s = di::black_box((char const*) "Hello");
    auto const* t = di::black_box((char const*) "ell");
    auto const* v = di::black_box((char const*) "lll");
    auto const* u = di::black_box((char const*) "Helloo");
    auto const* e = di::black_box((char const*) "");

    auto const* r1 = do_strstr(s, t);
    auto const* e1 = s + 1;
    ASSERT_EQ(r1, e1);

    auto const* r2 = do_strstr(s, v);
    auto e2 = nullptr;
    ASSERT_EQ(r2, e2);

    auto const* r3 = do_strstr(s, u);
    auto e3 = nullptr;
    ASSERT_EQ(r3, e3);

    auto const* r4 = do_strstr(s, e);
    auto const* e4 = s;
    ASSERT_EQ(r4, e4);

    auto const* r5 = do_strstr(e, e);
    auto const* e5 = e;
    ASSERT_EQ(r5, e5);
}

static void strlen_() {
    auto const* s = di::black_box((char const*) "Hello");
    auto const* e = di::black_box((char const*) "");

    auto r1 = do_strlen(s);
    auto e1 = 5u;
    ASSERT_EQ(r1, e1);

    auto r2 = do_strlen(e);
    auto e2 = 0u;
    ASSERT_EQ(r2, e2);
}

static void memcpy_() {
    auto src = di::black_box(di::Array { 4_b, 5_b, 6_b, 7_b });
    auto dst = di::black_box(di::Array { 8_b, 9_b, 10_b, 11_b });

    ASSERT_EQ(dst.data() + 1, do_memcpy(dst.data() + 1, src.data(), 2));

    auto e1 = di::Array { 8_b, 4_b, 5_b, 11_b };
    ASSERT_EQ(dst, e1);
}

static void memmove_() {
    auto bytes = di::black_box(di::Array { 4_b, 5_b, 6_b, 7_b, 8_b });
    ASSERT_EQ(bytes.data(), do_memmove(bytes.data(), bytes.data() + 1, 2));

    auto e1 = di::Array { 5_b, 6_b, 6_b, 7_b, 8_b };
    ASSERT_EQ(bytes, e1);

    ASSERT_EQ(bytes.data() + 3, do_memmove(bytes.data() + 3, bytes.data() + 2, 2));

    auto e2 = di::Array { 5_b, 6_b, 6_b, 6_b, 7_b };
    ASSERT_EQ(bytes, e2);
}

static void memset_() {
    auto bytes = di::black_box(di::Array { 4_b, 5_b, 6_b, 7_b });
    ASSERT_EQ(bytes.data(), do_memset(bytes.data(), 9_b, 3));

    auto ex1 = di::Array { 9_b, 9_b, 9_b, 7_b };
    ASSERT_EQ(bytes, ex1);
}

static void memcmp_() {
    auto a = di::black_box(di::Array { 4_b, 5_b, 6_b, 7_b });
    auto b = di::black_box(di::Array { 4_b, 5_b, 6_b, 7_b });
    auto c = di::black_box(di::Array { 4_b, 5_b, 6_b, 8_b });

    ASSERT_EQ(do_memcmp(a.data(), b.data(), a.size()), 0);
    ASSERT_LT(do_memcmp(a.data(), c.data(), a.size()), 0);
    ASSERT_GT(do_memcmp(c.data(), a.data(), a.size()), 0);
}

TEST(string_h, strcat_)
TEST(string_h, strncat_)
TEST(string_h, strcmp_)
TEST(string_h, strncmp_)
TEST(string_h, strcpy_)
TEST(string_h, strncpy_)
TEST(string_h, strchr_)
TEST(string_h, strrchr_)
TEST(string_h, strstr_)
TEST(string_h, strlen_)
TEST(string_h, memcpy_)
TEST(string_h, memmove_)
TEST(string_h, memset_)
TEST(string_h, memcmp_)
