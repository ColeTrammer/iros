#include <dius/system/system_call.h>
#include <errno.h>
#include <unistd.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html
extern "C" ssize_t read(int fd, void* buffer, size_t count) {
    auto result = dius::system::system_call<ssize_t>(dius::system::Number::read, fd, buffer, count);
    if (!result) {
        errno = int(result.error());
        return -1;
    }
    return *result;
}
