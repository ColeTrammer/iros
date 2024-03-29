#include <dius/system/system_call.h>
#include <errno.h>
#include <unistd.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/lseek.html
extern "C" off_t lseek(int fd, off_t offset, int whence) {
    auto result = dius::system::system_call<off_t>(dius::system::Number::lseek, fd, offset, whence);
    if (!result) {
        errno = int(result.error());
        return -1;
    }
    return *result;
}
