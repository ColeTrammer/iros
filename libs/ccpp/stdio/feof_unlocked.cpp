#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

// NOTE: this is an extension of feof(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/feof.html
extern "C" int feof_unlocked(FILE* file) {
    auto& inner = file->get_unlocked();
    return inner.status == ccpp::Status::Eof ? 1 : 0;
}
