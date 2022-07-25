#include <liim/container/array.h>
#include <liim/container/new_vector.h>
#include <liim/container/path_view.h>
#include <test/test.h>

constexpr void compare() {
    auto paths = make_vector({
        "/etc/resolv.conf"_pv,
        "/etc/passwd"_pv,
        "/usr"_pv,
        "/usr/include/unistd.h"_pv,
        "/"_pv,
        "./.bashrc"_pv,
        "./Downloads/"_pv,
        "CMakeLists.txt"_pv,
        ""_pv,
        "/usr/include/"_pv,
    });

    auto sorted_paths = make_vector({
        ""_pv,
        "./.bashrc"_pv,
        "./Downloads/"_pv,
        "/"_pv,
        "/etc/passwd"_pv,
        "/etc/resolv.conf"_pv,
        "/usr"_pv,
        "/usr/include/"_pv,
        "/usr/include/unistd.h"_pv,
        "CMakeLists.txt"_pv,
    });

    Alg::sort(paths);

    EXPECT_EQ(paths, sorted_paths);
}

constexpr void equal() {
    EXPECT_EQ("/a/b/ttt"_pv, "/a/b/ttt"_pv);
    EXPECT_EQ("/a/b/ttt"_pv, "/a/b/ttt/"_pv);
    EXPECT_EQ("//////a////b///ttt"_pv, "/a/b/ttt/"_pv);

    EXPECT_NOT_EQ("a/b/ttt"_pv, "/a/b/ttt"_pv);
    EXPECT_NOT_EQ("a/b/"_pv, "/a/b/ttt"_pv);

    EXPECT_EQ("./a/././b//."_pv, "./a/b"_pv);

    EXPECT_NOT_EQ("/a/b/.."_pv, "/a/"_pv);
}

constexpr void extension() {
    EXPECT_EQ("/a/b/test.txt"_pv.extension(), "txt");
    EXPECT_EQ("/a/b/test.txt.gz"_pv.extension(), "gz");
    EXPECT_EQ("/a/b/.bashrc"_pv.extension(), None {});
    EXPECT_EQ("/a/b/a.txt/"_pv.extension(), None {});
    EXPECT_EQ("/a/b/test"_pv.extension(), None {});
    EXPECT_EQ("/a/b/test."_pv.extension(), "");
}

constexpr void filename() {
    EXPECT_EQ("/a/b/xyz.abc"_pv.filename(), "xyz.abc");
    EXPECT_EQ("a/xyz.abc"_pv.filename(), "xyz.abc");
    EXPECT_EQ("test"_pv.filename(), "test");
    EXPECT_EQ("test/"_pv.filename(), None {});
    EXPECT_EQ("/test/"_pv.filename(), None {});
    EXPECT_EQ("/"_pv.filename(), None {});
    EXPECT_EQ(""_pv.filename(), None {});
}

constexpr void is_absolute() {
    EXPECT("///a/b/c"_pv.is_absolute());
    EXPECT(!"///a/b/c"_pv.is_relative());

    EXPECT(!"a/b/c"_pv.is_absolute());
    EXPECT("a/b/c"_pv.is_relative());
}

constexpr void iterator() {
    auto do_test = [](const auto& path_view, const auto& expected_components) {
        EXPECT_EQ(collect_vector(path_view), collect_vector(expected_components));
        EXPECT_EQ(collect_vector(reversed(path_view)), collect_vector(reversed(expected_components)));
    };

    do_test("//a//b//test.txt"_pv, Array { "/", "a", "b", "test.txt" });
    do_test("a//b//test.txt"_pv, Array { "a", "b", "test.txt" });
    do_test("a//b////"_pv, Array { "a", "b" });
    do_test(".//a//b////"_pv, Array { ".", "a", "b" });
    do_test("./a/b/.."_pv, Array { ".", "a", "b", ".." });
    do_test("./a/././b/./../."_pv, Array { ".", "a", "b", ".." });
}

constexpr void parent() {
    EXPECT_EQ("/a/b/test.txt"_pv.parent_path(), "/a/b"_pv);
    EXPECT_EQ("/a/b/"_pv.parent_path(), "/a"_pv);
    EXPECT_EQ("/a"_pv.parent_path(), "/"_pv);
    EXPECT_EQ("/"_pv.parent_path(), None {});

    EXPECT_EQ("a/b/test.txt"_pv.parent_path(), "a/b"_pv);
    EXPECT_EQ("a/b/"_pv.parent_path(), "a"_pv);
    EXPECT_EQ("a"_pv.parent_path(), None {});

    EXPECT_EQ("./a/b/test.txt"_pv.parent_path(), "./a/b"_pv);
    EXPECT_EQ("./a/b/"_pv.parent_path(), "./a"_pv);
    EXPECT_EQ("./a"_pv.parent_path(), "."_pv);
    EXPECT_EQ("."_pv.parent_path(), None {});
}

constexpr void stem() {
    EXPECT_EQ("/a/b/test.txt"_pv.stem(), "test");
    EXPECT_EQ("/a/b/test.txt.gz"_pv.stem(), "test.txt");
    EXPECT_EQ("/a/b/.bashrc"_pv.stem(), ".bashrc");
    EXPECT_EQ("/a/b/a.txt/"_pv.stem(), None {});
    EXPECT_EQ("/a/b/test"_pv.stem(), "test");
    EXPECT_EQ("/a/b/test."_pv.stem(), "test");
}

TEST_CONSTEXPR(pathview, compare, compare)
TEST_CONSTEXPR(pathview, equal, equal)
TEST_CONSTEXPR(pathview, extension, extension)
TEST_CONSTEXPR(pathview, filename, filename)
TEST_CONSTEXPR(pathview, is_absolute, is_absolute)
TEST_CONSTEXPR(pathview, iterator, iterator)
TEST_CONSTEXPR(pathview, parent, parent)
TEST_CONSTEXPR(pathview, stem, stem)
