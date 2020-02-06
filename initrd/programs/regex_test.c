#include <assert.h>
#include <regex.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <regex> <string>\n", argv[0]);
        return 1;
    }

    regex_t regex;
    int ret = regcomp(&regex, argv[1], 0);
    if (ret != 0) {
        char error_string[1024];
        regerror(ret, &regex, error_string, 1024);
        fprintf(stderr, "regcomp: %s\n", error_string);
        regfree(&regex);
        return 1;
    }

    size_t nmatch = regex.re_nsub + 1;
    regmatch_t matches[nmatch];
    int result = regexec(&regex, argv[2], nmatch, matches, 0);
    if (result != 0) {
        assert(result == REG_NOMATCH);
        fprintf(stderr, "regexec: no match '%s' '%s'\n", argv[1], argv[2]);
        regfree(&regex);
        return 1;
    }

    fprintf(stderr, "--- matches ---\n");
    for (size_t i = 0; i < nmatch; i++) {
        regmatch_t match = matches[i];
        if (match.rm_so == (regoff_t) -1) {
            printf("No match for sub regex: %lu\n", i);
            continue;
        }

        char *start = argv[2] + match.rm_so;
        char *end = argv[2] + match.rm_eo;
        char save = *end;
        *end = '\0';
        printf("%lu: '%s'\n", i, start);
        *end = save;
    }

    regfree(&regex);
    return 0;
}