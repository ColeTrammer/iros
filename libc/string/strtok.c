#include <string.h>

static char *save;

char *strtok(char *str, const char *delim) {
    char *start = str;
    if (str == NULL) {
        if (save == NULL) {
            return NULL;
        }

        start = save;
    }

    size_t i = 0;
    while (start[i] != '\0') {
        for (size_t j = 0; delim[j] != '\0'; j++) {
            if (start[i] == delim[j]) {
                if (i == 0) {
                    start++;
                    break;
                } else {
                    start[i] = '\0';
                    goto end;
                }
            }
        }

        i++;
    }

    if (start[0] == '\0') {
        return NULL;
    }

    end:
    save = start[i] == '\0' ? NULL : start + i;
    return start;
}