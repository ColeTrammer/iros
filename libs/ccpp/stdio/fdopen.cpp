#include <ccpp/bits/file_implementation.h>
#include <di/container/allocator/allocate_many.h>
#include <di/container/allocator/deallocate_many.h>
#include <dius/sync_file.h>
#include <string.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fdopen.html
extern "C" FILE* fdopen(int fd, char const* mode) {
    // FIXME: this may need to change the underlying file descriptor or validate it is read/writable.
    auto mode_sv = di::TransparentStringView(mode, mode + strlen(mode));
    auto permissions = Permissions::None;
    if (mode_sv == "r"_tsv || mode_sv == "rb"_tsv) {
        permissions = Permissions::Readable;
    } else if (mode_sv == "w"_tsv || mode_sv == "wb"_tsv) {
        permissions = Permissions::Writable;
    } else if (mode_sv == "a"_tsv || mode_sv == "ab"_tsv) {
        permissions = Permissions::Writable;
    } else if (mode_sv == "r+"_tsv || mode_sv == "r+b"_tsv || mode_sv == "rb+"_tsv) {
        permissions = Permissions::Readable | Permissions::Writable;
    } else if (mode_sv == "w+"_tsv || mode_sv == "w+b"_tsv || mode_sv == "wb+"_tsv) {
        permissions = Permissions::Readable | Permissions::Writable;
    } else if (mode_sv == "a+"_tsv || mode_sv == "a+b"_tsv || mode_sv == "ab+"_tsv) {
        permissions = Permissions::Readable | Permissions::Writable;
    } else {
        errno = EINVAL;
        return nullptr;
    }

    auto file = dius::SyncFile(dius::SyncFile::Owned::No, fd);

    auto allocator = MallocAllocator {};
    auto* buffer = STDIO_TRY_OR_NULL(di::allocate_many<byte>(allocator, BUFSIZ)).data;
    auto guard = di::ScopeExit([&] {
        di::deallocate_many<byte>(allocator, buffer, BUFSIZ);
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
