#include <di/prelude.h>
#include <test/test.h>

constexpr void iteration() {
    auto do_test = [](auto&& path_view, auto&& expected_components) {
        auto r1 = path_view | di::to<di::Vector>();
        auto ex1 = expected_components | di::to<di::Vector>();
        ASSERT_EQ(r1, ex1);

        auto r2 = path_view | di::reverse | di::to<di::Vector>();
        auto ex2 = expected_components | di::reverse | di::to<di::Vector>();
        ASSERT_EQ(r2, ex2);
    };

    do_test("//a//b//test.txt"_pv, Array { "/"_tsv, "a"_tsv, "b"_tsv, "test.txt"_tsv });
    do_test("a//b//test.txt"_pv, Array { "a"_tsv, "b"_tsv, "test.txt"_tsv });
    do_test("a//b////"_pv, Array { "a"_tsv, "b"_tsv });
    do_test(".//a//b////"_pv, Array { "."_tsv, "a"_tsv, "b"_tsv });
    do_test("./a/b/.."_pv, Array { "."_tsv, "a"_tsv, "b"_tsv, ".."_tsv });
    // do_test("./a/././b/./../."_pv, Array { "."_tsv, "a"_tsv, "b"_tsv, ".."_tsv });
}

constexpr void equal() {
    ASSERT_EQ("/a/b/ttt"_pv, "/a/b/ttt"_pv);
    ASSERT_EQ("/a/b/ttt"_pv, "/a/b/ttt/"_pv);
    ASSERT_EQ("//////a////b///ttt"_pv, "/a/b/ttt/"_pv);

    ASSERT_NOT_EQ("a/b/ttt"_pv, "/a/b/ttt"_pv);
    ASSERT_NOT_EQ("a/b/"_pv, "/a/b/ttt"_pv);

    // ASSERT_EQ("./a/././b//."_pv, "./a/b"_pv);

    ASSERT_NOT_EQ("/a/b/.."_pv, "/a/"_pv);
}

constexpr void extension() {
    ASSERT_EQ("/a/b/test.txt"_pv.extension(), "txt"_tsv);
    ASSERT_EQ("/a/b/test.txt.gz"_pv.extension(), "gz"_tsv);
    ASSERT_EQ("/a/b/.bashrc"_pv.extension(), di::nullopt);
    ASSERT_EQ("/a/b/a.txt/"_pv.extension(), di::nullopt);
    ASSERT_EQ("/a/b/test"_pv.extension(), di::nullopt);
    ASSERT_EQ("/a/b/test."_pv.extension(), ""_tsv);
}

constexpr void filename() {
    ASSERT_EQ("/a/b/xyz.abc"_pv.filename(), "xyz.abc"_tsv);
    ASSERT_EQ("a/xyz.abc"_pv.filename(), "xyz.abc"_tsv);
    ASSERT_EQ("test"_pv.filename(), "test"_tsv);
    ASSERT_EQ("test/"_pv.filename(), di::nullopt);
    ASSERT_EQ("/test/"_pv.filename(), di::nullopt);
    ASSERT_EQ("/"_pv.filename(), di::nullopt);
    ASSERT_EQ(""_pv.filename(), di::nullopt);
}

constexpr void is_absolute() {
    ASSERT("///a/b/c"_pv.is_absolute());
    ASSERT(!"///a/b/c"_pv.is_relative());

    ASSERT(!"a/b/c"_pv.is_absolute());
    ASSERT("a/b/c"_pv.is_relative());
}

constexpr void parent() {
    ASSERT_EQ("/a/b/test.txt"_pv.parent_path(), "/a/b"_pv);
    ASSERT_EQ("/a/b/"_pv.parent_path(), "/a"_pv);
    ASSERT_EQ("/a"_pv.parent_path(), "/"_pv);
    ASSERT_EQ("/"_pv.parent_path(), di::nullopt);

    ASSERT_EQ("a/b/test.txt"_pv.parent_path(), "a/b"_pv);
    ASSERT_EQ("a/b/"_pv.parent_path(), "a"_pv);
    ASSERT_EQ("a"_pv.parent_path(), di::nullopt);

    ASSERT_EQ("./a/b/test.txt"_pv.parent_path(), "./a/b"_pv);
    ASSERT_EQ("./a/b/"_pv.parent_path(), "./a"_pv);
    ASSERT_EQ("./a"_pv.parent_path(), "."_pv);
    ASSERT_EQ("."_pv.parent_path(), di::nullopt);
}

constexpr void stem() {
    ASSERT_EQ("/a/b/test.txt"_pv.stem(), "test"_tsv);
    ASSERT_EQ("/a/b/test.txt.gz"_pv.stem(), "test.txt"_tsv);
    ASSERT_EQ("/a/b/.bashrc"_pv.stem(), ".bashrc"_tsv);
    ASSERT_EQ("/a/b/a.txt/"_pv.stem(), di::nullopt);
    ASSERT_EQ("/a/b/test"_pv.stem(), "test"_tsv);
    ASSERT_EQ("/a/b/test."_pv.stem(), "test"_tsv);
}

constexpr void starts_with() {
    ASSERT("/a/b/c"_pv.starts_with("/a/"_pv));
    ASSERT("/a/b/c"_pv.starts_with("///a///b///"_pv));
    ASSERT("/a/b/c"_pv.starts_with("///a///b///c///"_pv));
    ASSERT(!"/a/b/c"_pv.starts_with("a/b/"_pv));
    ASSERT(!"/a/b/c"_pv.starts_with("a/b/c/d"_pv));

    ASSERT("./a/b"_pv.starts_with("./a"_pv));
    ASSERT(!"./a/b"_pv.starts_with("a"_pv));
}

constexpr void ends_with() {
    ASSERT("/etc/resolv.conf"_pv.ends_with("resolv.conf"_pv));
    ASSERT("/etc/resolv.conf"_pv.ends_with("etc/resolv.conf"_pv));
    ASSERT("/etc/resolv.conf"_pv.ends_with("///etc///resolv.conf//"_pv));
    ASSERT(!"/etc/resolv.conf"_pv.ends_with(".conf"_pv));
    ASSERT(!"/etc/resolv.conf"_pv.ends_with("/etc"_pv));
}

constexpr void filename_ends_with() {
    ASSERT("/opt/bash.tar.gz"_pv.filename_ends_with(".tar.gz"_tsv));
    ASSERT(!"/opt/bash.tar.gz/"_pv.filename_ends_with(".tar.gz"_tsv));
    ASSERT(!"/opt/bash.gz/"_pv.filename_ends_with(".tar.gz"_tsv));
}

TEST_CONSTEXPR(container_path, iteration, iteration)
TEST_CONSTEXPR(container_path, equal, equal)
TEST_CONSTEXPR(container_path, extension, extension)
TEST_CONSTEXPR(container_path, filename, filename)
TEST_CONSTEXPR(container_path, is_absolute, is_absolute)
TEST_CONSTEXPR(container_path, parent, parent)
TEST_CONSTEXPR(container_path, stem, stem)
TEST_CONSTEXPR(container_path, starts_with, starts_with)
TEST_CONSTEXPR(container_path, ends_with, ends_with)
TEST_CONSTEXPR(container_path, filename_ends_with, filename_ends_with)