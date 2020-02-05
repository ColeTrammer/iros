#include <regex.h>

extern "C" int regexec(const regex_t *__restrict regex, const char *__restrict str, size_t nmatch, regmatch_t __restrict matches[],
                       int eflags) {
    (void) regex;
    (void) str;
    (void) nmatch;
    (void) matches;
    (void) eflags;
    return REG_NOMATCH;
}
