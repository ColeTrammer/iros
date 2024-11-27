#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getc_unlocked.html
extern "C" auto getc_unlocked(FILE* file) -> int {
    return fgetc_unlocked(file);
}
