#include <sys/stat.h>

#include <errno.h>

#include "dius/system/system_call.h"

extern "C" auto stat(char const* __restrict path, struct stat* __restrict info) -> int {
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
