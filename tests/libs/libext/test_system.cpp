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

    auto result = Ext::System::realpath("/proc/self/exed");
    EXPECT_EQ(result.error().value(), ENOENT);

    result = Ext::System::realpath("/proc/self/exe");
    EXPECT(result.has_value());
    EXPECT(result.value().ends_with("test_libext"));
}
