#include <liim/container/path.h>
#include <test/test.h>

TEST(path, basic) {
    auto p = "/etc/resolv.conf"_p;
    EXPECT_EQ(p, "/etc/././//resolv.conf"_p);

    EXPECT_EQ(p.filename(), "resolv.conf"sv);
    EXPECT(p.is_absolute());

    EXPECT("a/b/CMakeLists.txt"_p.is_relative());
}

TEST(path, join) {
    auto p = "/etc"_p;
    auto w = p.join("{}_{}.txt", 1, 2);
    EXPECT_EQ(w, "/etc/1_2.txt"_pv);

    auto v = "/etc/"_p.join("hello.txt"_pv);
    EXPECT_EQ(v, "/etc/hello.txt"_pv);

    auto x = p.join("/etc/resolv.conf"_pv);
    EXPECT_EQ(x, "/etc/resolv.conf"_pv);
}
