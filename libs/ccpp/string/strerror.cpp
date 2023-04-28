#include <dius/error.h>
#include <string.h>

extern "C" char* strerror(int errnum) {
    auto error = dius::PosixCode(dius::PosixError(errnum));
    return const_cast<char*>(reinterpret_cast<char const*>(error.message().data()));
}
