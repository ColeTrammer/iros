#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getc.html
extern "C" auto getc(FILE* file) -> int {
    return fgetc(file);
}
