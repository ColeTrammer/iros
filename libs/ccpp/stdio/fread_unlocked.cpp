#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fread(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fread.html
extern "C" size_t fread_unlocked(void* __restrict buffer, size_t size, size_t count, FILE* __restrict file) {
    for (auto nbytes : di::range(count * size)) {
        auto ch = fgetc_unlocked(file);
        if (ch == EOF) {
            return nbytes / size;
        }
        static_cast<byte*>(buffer)[nbytes] = byte(ch);
    }
    return count;
}
