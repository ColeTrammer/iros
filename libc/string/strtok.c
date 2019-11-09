#include <string.h>

static char *save;

static char *save = NULL;

char *strtok(char *str, const char *delim) {
    char *start = str;
    if (str == NULL) {
        if (save == NULL) {
            return NULL;
        }

        start = save;
    }

    size_t i = 0;
skip_inc_i:
    while (start[i] != '\0') {
        for (size_t j = 0; delim[j] != '\0'; j++) {
            if (start[i] == delim[j]) {
                if (i == 0) {
                    start++;
                    goto skip_inc_i;
                } else {
                    start[i] = '\0';
                    goto end;
                }
            }
        }

        i++;
    }

    if (start[0] == '\0') {
        save = NULL;
        return NULL;
    }

end:
    save = i == 0 || start[i] == '\0' ? NULL : start + i + 1;
    return start;
}