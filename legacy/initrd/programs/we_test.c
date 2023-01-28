/* ch12-glob.c --- demonstrate glob(). */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

int main(/* int argc, char **argv */) {
    wordexp_t results = { 0 };

    // if (argc != 2) {
    //     fprintf(stderr, "usage: %s <words>\n", argv[0]);
    //     return 0;
    // }

    const char *s = "\"asdf\"";
    int ret;
    if ((ret = wordexp(s, &results, 0))) {
        printf("Error: %d\n", ret);
        return 1;
    }

    printf("|%s|\n", s);

    for (size_t j = 0; j < results.we_wordc; j++) {
        printf("|%s|\n", results.we_wordv[j]);
    }

    wordfree(&results);
    return 0;
}
