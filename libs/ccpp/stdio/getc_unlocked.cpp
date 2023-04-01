#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getc_unlocked.html
extern "C" int getc_unlocked(FILE* file) {
    return fgetc_unlocked(file);
}
