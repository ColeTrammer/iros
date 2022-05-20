#include <ext/path.h>
#include <ext/system.h>
#include <test/test.h>

TEST(system, realpath) {
    auto result = Ext::realpath("/proc/self/exed");
    EXPECT(result.is_error());
    EXPECT_EQ(result.error().system_call(), "realpath");
    EXPECT_EQ(result.error().error_code(), ENOENT);

    result = Ext::realpath("/proc/self/exe");
    EXPECT(result.is_ok());
    EXPECT(result.value().ends_with("test_libext"));
}
