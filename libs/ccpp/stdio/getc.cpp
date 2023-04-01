#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getc.html
extern "C" int getc(FILE* file) {
    return fgetc(file);
}
