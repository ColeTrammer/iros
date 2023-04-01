#include <ccpp/bits/file_implementation.h>
#include <string.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/tmpfile.html
extern "C" FILE* tmpfile(void) {
    auto file = STDIO_TRY_OR_NULL(dius::open_tempory_file());

    auto permissions = Permissions::Readable | Permissions::Writable;

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
