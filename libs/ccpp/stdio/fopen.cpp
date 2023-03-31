#include <ccpp/bits/file_implementation.h>
#include <string.h>

namespace ccpp {
extern "C" FILE* fopen(char const* path, char const* mode) {
    auto mode_sv = di::TransparentStringView(mode, mode + strlen(mode));
    auto open_mode = dius::OpenMode::Readonly;
    auto permissions = Permissions::None;
    if (mode_sv == "r"_tsv || mode_sv == "rb"_tsv) {
        open_mode = dius::OpenMode::Readonly;
        permissions = Permissions::Readable;
    } else if (mode_sv == "w"_tsv || mode_sv == "wb"_tsv) {
        open_mode = dius::OpenMode::Readonly;
        permissions = Permissions::Writable;
    } else if (mode_sv == "a"_tsv || mode_sv == "ab"_tsv) {
        open_mode = dius::OpenMode::AppendOnly;
        permissions = Permissions::Writable;
    } else if (mode_sv == "r+"_tsv || mode_sv == "r+b"_tsv || mode_sv == "rb+"_tsv) {
        open_mode = dius::OpenMode::ReadWrite;
        permissions = Permissions::Readable | Permissions::Writable;
    } else if (mode_sv == "w+"_tsv || mode_sv == "w+b"_tsv || mode_sv == "wb+"_tsv) {
        open_mode = dius::OpenMode::ReadWriteClobber;
        permissions = Permissions::Readable | Permissions::Writable;
    } else if (mode_sv == "a+"_tsv || mode_sv == "a+b"_tsv || mode_sv == "ab+"_tsv) {
        open_mode = dius::OpenMode::AppendReadWrite;
        permissions = Permissions::Readable | Permissions::Writable;
    } else {
        errno = EINVAL;
        return nullptr;
    }

    auto path_sv = di::TransparentStringView(path, path + strlen(path));
    auto file = STDIO_TRY_OR_NULL(dius::open_sync(path_sv, open_mode));

    auto* buffer = STDIO_TRY_OR_NULL(MallocAllocator<byte>().allocate(BUFSIZ)).data;
    auto guard = di::ScopeExit([&] {
        MallocAllocator<byte>().deallocate(buffer, BUFSIZ);
    });

    auto handle = FileHandle(new (std::nothrow) FILE());
    if (!handle) {
        errno = ENOMEM;
        return nullptr;
    }

    auto& data = handle->get_unlocked();
    data.file = di::move(file);

    data.buffer = buffer;
    data.buffer_capacity = BUFSIZ;
    data.buffer_ownership = BufferOwnership::Owned;
    guard.release();

    data.permissions = permissions;
    data.buffer_mode = BufferMode::FullBuffered;
    return handle.release();
}
}
