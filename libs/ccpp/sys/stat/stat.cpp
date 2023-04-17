#include <dius/system/system_call.h>
#include <errno.h>
#include <sys/stat.h>

extern "C" int stat(char const* __restrict path, struct stat* __restrict info) {
#ifdef __linux__
    auto result = dius::system::system_call<int>(dius::system::Number::fstatat64, AT_FDCWD, path, info, 0);
    if (!result) {
        errno = int(result.error());
        return -1;
    }
    return *result;
#else
    (void) path;
    (void) info;

    return 0;
#endif
}
