#include <di/function/prelude.h>
#include <dius/filesystem/operations.h>
#include <dius/filesystem/prelude.h>
#include <dius/filesystem/query/is_block_file.h>
#include <dius/filesystem/recursive_directory_iterator.h>
#include <dius/print.h>
#include <dius/sync_file.h>
#include <dius/system/process.h>
#include <dius/test/prelude.h>

namespace filesystem {
constexpr auto temp_directory_path = "/tmp/test_dius"_pv;

static auto remove_temp_directory() {
    // For now, only linux supports removing directories.
#ifdef DIUS_PLATFORM_LINUX
    // FIXME: add PATH support to the dius runtime process implementation, so we don't need an absolute path.
    auto args = di::Array { "/usr/bin/env"_tsv.to_owned(), "rm"_tsv.to_owned(), "-rf"_tsv.to_owned(),
                            temp_directory_path.data().to_owned() } |
                di::to<di::Vector>();
    ASSERT(dius::system::Process(di::move(args)).spawn_and_wait());
#endif
}

static auto setup_temp_directory() {
    remove_temp_directory();

    ASSERT(dius::fs::create_directory(temp_directory_path));

    return di::ScopeExit([] {
        remove_temp_directory();
    });
}

static void basic() {
    auto guard = setup_temp_directory();

    auto r1 = dius::fs::exists(temp_directory_path);
    ASSERT_EQ(r1, true);

    ASSERT_EQ(dius::fs::create_regular_file(temp_directory_path.to_owned() / "a"_pv), true);
    ASSERT_EQ(dius::fs::create_directory(temp_directory_path.to_owned() / "b"_pv), true);
    ASSERT_EQ(dius::fs::create_regular_file(temp_directory_path.to_owned() / "b/c"_pv), true);
    ASSERT_EQ(dius::fs::create_regular_file(temp_directory_path.to_owned() / "b/c"_pv), false);

    auto r2 = dius::fs::exists(temp_directory_path.to_owned() / "a"_pv);
    ASSERT_EQ(r2, true);

    auto r3 = dius::fs::exists(temp_directory_path.to_owned() / "b"_pv);
    ASSERT_EQ(r3, true);

    auto r4 = dius::fs::exists(temp_directory_path.to_owned() / "b/c"_pv);
    ASSERT_EQ(r4, true);

    auto r5 = dius::fs::is_directory(temp_directory_path.to_owned() / "b"_pv);
    ASSERT_EQ(r5, true);

    // Test '..' and '.' resolution works.
    auto r6 = dius::fs::is_regular_file(temp_directory_path.to_owned() / "b/../b/./c"_pv);
    ASSERT_EQ(r6, true);

    auto directory_it = *di::create<dius::fs::RecursiveDirectoryIterator>(temp_directory_path.to_owned());
    auto r7 = di::Vector<di::Path> {};
    for (auto&& entry : directory_it) {
        auto path = entry->path().clone();
        ASSERT(path.starts_with(temp_directory_path));
        r7.push_back(path.clone());
    }

    di::sort(r7);

    auto e6 = di::Array {
        temp_directory_path.to_owned() / "a"_pv,
        temp_directory_path.to_owned() / "b"_pv,
        temp_directory_path.to_owned() / "b/c"_pv,
    } | di::to<di::Vector>();
    ASSERT_EQ(r7, e6);

    auto buffer = di::Array<byte, 8192> {};
    di::copy(di::range(buffer.size()) | di::transform([](usize x) {
                 return byte(x);
             }),
             buffer.data());

    {
        auto file = *dius::open_sync(temp_directory_path.to_owned() / "a"_pv, dius::OpenMode::WriteClobber);
        ASSERT(file.write_exactly(buffer.span()));
    }

    {
        auto file = *dius::open_sync(temp_directory_path.to_owned() / "a"_pv, dius::OpenMode::Readonly);
        auto r8 = di::Array<byte, 8192> {};
        ASSERT(file.read_exactly(r8.span()));
        ASSERT_EQ(r8, buffer);
    }
}

TEST(filesystem, basic)
}
