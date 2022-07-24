#include <liim/container/path.h>
#include <test/test.h>

TEST(path, basic) {
    auto p = "/etc/resolv.conf"_p;
    EXPECT_EQ(p, "/etc/././//resolv.conf"_p);

    EXPECT_EQ(p.filename(), "resolv.conf"sv);
    EXPECT(p.is_absolute());

    EXPECT("a/b/CMakeLists.txt"_p.is_relative());
}
