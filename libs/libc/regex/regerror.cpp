#include <regex.h>

extern "C" size_t regerror(int error, const regex_t *__restrict regex, char *__restrict buf, size_t buffer_len) {
    (void) error;
    (void) regex;
    (void) buffer_len;
    if (buf) {
        *buf = '\0';
    }
    return 0;
}
