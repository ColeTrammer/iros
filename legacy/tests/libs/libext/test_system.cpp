#include <ext/system.h>
#include <liim/container/path.h>
#include <sys/mount.h>
#include <test/test.h>
#include <unistd.h>

TEST(system, realpath) {
#ifdef __iros__
    if (getpid() == 1) {
        EXPECT_EQ(mount("", "/proc", "procfs", 0, nullptr), 0);
    }
#endif

    auto result = Ext::System::realpath("/proc/self/exed"_pv);
    EXPECT_EQ(result.error().value(), ENOENT);

    result = Ext::System::realpath("/proc/self/exe"_pv);
    EXPECT(result.has_value());
    EXPECT_EQ(result.value().filename(), "test_libext"sv);
}
