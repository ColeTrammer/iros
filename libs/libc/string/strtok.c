#include <string.h>

static char *save;

char *strtok(char *str, const char *delim) {
    return strtok_r(str, delim, &save);
}
