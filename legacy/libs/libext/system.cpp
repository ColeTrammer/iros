#include <ext/system.h>
#include <fcntl.h>
#include <liim/container/path.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <unistd.h>

namespace Ext::System {
Result<Path> realpath(PathView view) {
    auto path = String(view.data());
    char* result = ::realpath(path.string(), nullptr);
    if (!result) {
        return Err(make_system_error_from_errno());
    }
    auto ret = Path::create(String(result));
    free(result);
    return ret;
}

Result<int> open(PathView view, OpenMode open_mode, mode_t create_mode) {
    auto path = String(view.data());
    int fd = ::open(path.string(), to_underlying(open_mode), create_mode);
    if (fd < 0) {
        return Err(make_system_error_from_errno());
    }
    return fd;
}

Result<void*> mmap(void* addr, size_t length, Protection protection, MapFlags flags, Option<int> fd, off_t offset) {
    auto* result = ::mmap(addr, length, to_underlying(protection), to_underlying(flags), fd.value_or(-1), offset);
    if (result == MAP_FAILED) {
        return Err(make_system_error_from_errno());
    }
    return result;
}

Result<struct stat> fstat(int fd) {
    struct stat st;
    int result = ::fstat(fd, &st);
    if (result < 0) {
        return Err(make_system_error_from_errno());
    }
    return st;
}

Result<void> chdir(PathView view) {
    auto path = String(view.data());
    int result = ::chdir(path.string());
    if (result < 0) {
        return Err(make_system_error_from_errno());
    }
    return {};
}

Result<bool> exists(PathView view) {
    auto path = String(view.data());
    int result = ::access(path.string(), F_OK);
    if (result < 0 && errno != ENOENT) {
        return Err(make_system_error_from_errno());
    }
    return result == F_OK;
}
}
