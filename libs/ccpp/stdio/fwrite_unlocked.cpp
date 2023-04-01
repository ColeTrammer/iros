#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fwrite(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fwrite.html
extern "C" size_t fwrite_unlocked(void const* __restrict buffer, size_t size, size_t count, FILE* __restrict file) {
    for (auto nbytes : di::range(count * size)) {
        auto ch = fputc_unlocked(di::to_integer<int>(static_cast<byte const*>(buffer)[nbytes]), file);
        if (ch == EOF) {
            return nbytes / size;
        }
    }
    return count;
}
