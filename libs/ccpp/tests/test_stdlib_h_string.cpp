#include <dius/test/prelude.h>
#include <stdlib.h>

[[gnu::noinline]] static int do_atoi(char const* s) {
    return atoi(s);
}

static void atoi_() {
    ASSERT_EQ(do_atoi("   -12345HI"), -12345);
    ASSERT_EQ(do_atoi("   +321HI12345"), 12345);
    ASSERT_EQ(do_atoi("0"), 0);
    ASSERT_EQ(do_atoi("0042"), 42);
    ASSERT_EQ(do_atoi("0x2A"), 0);
    ASSERT_EQ(do_atoi("HI"), 0);
    ASSERT_EQ(do_atoi("2147483647"), 2147483647);
    ASSERT_EQ(do_atoi("-2147483648"), -2147483648);
}

TEST(stdlib_h, atoi_)
