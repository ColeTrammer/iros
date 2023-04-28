#include <di/container/string/prelude.h>
#include <dius/test/prelude.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace stdlib_h {
[[gnu::noinline]] static int do_atoi(char const* s) {
    return atoi(s);
}

[[gnu::noinline]] static long do_atol(char const* s) {
    return atol(s);
}

[[gnu::noinline]] static long do_atoll(char const* s) {
    return atoll(s);
}

[[gnu::noinline]] static long do_strtol(char const* s, char** end, int radix) {
    *end = nullptr;
    errno = 0;
    return strtol(s, end, radix);
}

[[gnu::noinline]] static long long do_strtoll(char const* s, char** end, int radix) {
    *end = nullptr;
    errno = 0;
    return strtoll(s, end, radix);
}

[[gnu::noinline]] static unsigned long do_strtoul(char const* s, char** end, int radix) {
    *end = nullptr;
    errno = 0;
    return strtoul(s, end, radix);
}

[[gnu::noinline]] static unsigned long long do_strtoull(char const* s, char** end, int radix) {
    *end = nullptr;
    errno = 0;
    return strtoull(s, end, radix);
}

static void atoi_() {
    ASSERT_EQ(do_atoi("   -12345HI"), -12345);
    ASSERT_EQ(do_atoi("   +12345HI"), 12345);
    ASSERT_EQ(do_atoi("0"), 0);
    ASSERT_EQ(do_atoi("0042"), 42);
    ASSERT_EQ(do_atoi("0x2A"), 0);
    ASSERT_EQ(do_atoi("HI"), 0);
    ASSERT_EQ(do_atoi("2147483647"), 2147483647);
    ASSERT_EQ(do_atoi("-2147483648"), -2147483648);
}

static void atol_() {
    ASSERT_EQ(do_atol("   -12345HI"), -12345);
    ASSERT_EQ(do_atol("   +12345HI"), 12345);
    ASSERT_EQ(do_atol("0"), 0);
    ASSERT_EQ(do_atol("0042"), 42);
    ASSERT_EQ(do_atol("0x2A"), 0);
    ASSERT_EQ(do_atol("HI"), 0);
    ASSERT_EQ(do_atol("9223372036854775807"), 9223372036854775807);
    ASSERT_EQ(do_atol("-9223372036854775808"), (long) -9223372036854775808u);
}

static void atoll_() {
    ASSERT_EQ(do_atoll("   -12345HI"), -12345);
    ASSERT_EQ(do_atoll("   +12345HI"), 12345);
    ASSERT_EQ(do_atoll("0"), 0);
    ASSERT_EQ(do_atoll("0042"), 42);
    ASSERT_EQ(do_atoll("0x2A"), 0);
    ASSERT_EQ(do_atoll("HI"), 0);
    ASSERT_EQ(do_atoll("9223372036854775807"), 9223372036854775807);
    ASSERT_EQ(do_atoll("-9223372036854775808"), (long long) -9223372036854775808u);
}

static di::TransparentStringView cstring_to_tsv(char const* ptr) {
    return di::TransparentStringView(ptr, strlen(ptr));
}

static void strtol_() {
    char* end = nullptr;
    ASSERT_EQ(do_strtol("   -12345HI", &end, 10), -12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtol("   +12345HI", &end, 10), 12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtol("0", &end, 10), 0);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("0042", &end, 10), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("0x2A", &end, 16), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("HI", &end, 10), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtol("9223372036854775807", &end, 10), 9223372036854775807);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("-9223372036854775808", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("92233720368547758079", &end, 10), 9223372036854775807);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("9223372036854775808", &end, 10), 9223372036854775807);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("-92233720368547758089", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("-9223372036854775809", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 2
    ASSERT_EQ(do_strtol("0b101010", &end, 2), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtol("101010", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("1010102", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), "2"_tsv);

    // Radix 35
    ASSERT_EQ(do_strtol("z1", &end, 35), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "z1"_tsv);

    // Radix 36
    ASSERT_EQ(do_strtol("z1", &end, 36), 1261);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 0
    ASSERT_EQ(do_strtol("0x2A", &end, 0), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("0b101010", &end, 0), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtol("0666", &end, 0), 0666);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtol("127", &end, 0), 127);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);
}

static void strtoll_() {
    char* end = nullptr;
    ASSERT_EQ(do_strtoll("   -12345HI", &end, 10), -12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoll("   +12345HI", &end, 10), 12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoll("0", &end, 10), 0);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("0042", &end, 10), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("0x2A", &end, 16), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("HI", &end, 10), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoll("9223372036854775807", &end, 10), 9223372036854775807);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("-9223372036854775808", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("92233720368547758079", &end, 10), 9223372036854775807);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("9223372036854775808", &end, 10), 9223372036854775807);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("-92233720368547758089", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("-9223372036854775809", &end, 10), (long long) -9223372036854775808u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 2
    ASSERT_EQ(do_strtoll("0b101010", &end, 2), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoll("101010", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("1010102", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), "2"_tsv);

    // Radix 35
    ASSERT_EQ(do_strtoll("z1", &end, 35), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "z1"_tsv);

    // Radix 36
    ASSERT_EQ(do_strtoll("z1", &end, 36), 1261);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 0
    ASSERT_EQ(do_strtoll("0x2A", &end, 0), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("0b101010", &end, 0), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoll("0666", &end, 0), 0666);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoll("127", &end, 0), 127);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);
}

static void strtoul_() {
    char* end = nullptr;
    ASSERT_EQ(do_strtoul("   -12345HI", &end, 10), -12345ull);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoul("   +12345HI", &end, 10), 12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoul("0", &end, 10), 0);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("0042", &end, 10), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("0x2A", &end, 16), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("HI", &end, 10), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoul("18446744073709551615", &end, 10), 18446744073709551615u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("18446744073709551616", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("18446744073709551617", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("-18446744073709551615", &end, 10), -18446744073709551615u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("-18446744073709551616", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 2
    ASSERT_EQ(do_strtoul("0b101010", &end, 2), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoul("101010", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("1010102", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), "2"_tsv);

    // Radix 35
    ASSERT_EQ(do_strtoul("z1", &end, 35), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "z1"_tsv);

    // Radix 36
    ASSERT_EQ(do_strtoul("z1", &end, 36), 1261);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 0
    ASSERT_EQ(do_strtoul("0x2A", &end, 0), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("0b101010", &end, 0), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoul("0666", &end, 0), 0666);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoul("127", &end, 0), 127);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);
}

static void strtoull_() {
    char* end = nullptr;
    ASSERT_EQ(do_strtoull("   -12345HI", &end, 10), -12345ull);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoull("   +12345HI", &end, 10), 12345);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoull("0", &end, 10), 0);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("0042", &end, 10), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("0x2A", &end, 16), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("HI", &end, 10), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "HI"_tsv);

    ASSERT_EQ(do_strtoull("18446744073709551615", &end, 10), 18446744073709551615u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("18446744073709551616", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("184467440737095516159", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("-18446744073709551615", &end, 10), -18446744073709551615u);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("-18446744073709551616", &end, 10), 18446744073709551615u);
    ASSERT_EQ(errno, ERANGE);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 2
    ASSERT_EQ(do_strtoull("0b101010", &end, 2), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoull("101010", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("1010102", &end, 2), 42);
    ASSERT_EQ(cstring_to_tsv(end), "2"_tsv);

    // Radix 35
    ASSERT_EQ(do_strtoull("z1", &end, 35), 0);
    ASSERT(errno == 0 || errno == EINVAL);
    ASSERT_EQ(cstring_to_tsv(end), "z1"_tsv);

    // Radix 36
    ASSERT_EQ(do_strtoull("z1", &end, 36), 1261);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    // Radix 0
    ASSERT_EQ(do_strtoull("0x2A", &end, 0), 42);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("0b101010", &end, 0), 0);
    ASSERT_EQ(cstring_to_tsv(end), "b101010"_tsv);

    ASSERT_EQ(do_strtoull("0666", &end, 0), 0666);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);

    ASSERT_EQ(do_strtoull("127", &end, 0), 127);
    ASSERT_EQ(cstring_to_tsv(end), ""_tsv);
}

TEST(stdlib_h, atoi_)
TEST(stdlib_h, atol_)
TEST(stdlib_h, atoll_)
TEST(stdlib_h, strtol_)
TEST(stdlib_h, strtoll_)
TEST(stdlib_h, strtoul_)
TEST(stdlib_h, strtoull_)
}
