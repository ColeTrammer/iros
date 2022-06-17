#include <ext/path.h>
#include <ext/system.h>
#include <sys/mount.h>
#include <test/test.h>
#include <unistd.h>

TEST(system, realpath) {
#ifdef __iros__
    if (getpid() == 1) {
        EXPECT_EQ(mount("", "/proc", "procfs", 0, nullptr), 0);
    }
#endif

    auto result = Ext::realpath("/proc/self/exed");
    EXPECT(result.is_error());
    EXPECT_EQ(result.error().system_call(), "realpath");
    EXPECT_EQ(result.error().error_code(), ENOENT);

    result = Ext::realpath("/proc/self/exe");
    EXPECT(result.has_value());
    EXPECT(result.value().ends_with("test_libext"));
}
