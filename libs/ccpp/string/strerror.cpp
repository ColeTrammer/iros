#include <di/platform/prelude.h>
#include <di/vocab/error/prelude.h>
#include <dius/error.h>
#include <string.h>

extern "C" char* strerror(int errnum) {
    auto error = di::GenericCode(di::BasicError(errnum));
    return const_cast<char*>(reinterpret_cast<char const*>(error.message().data()));
}
