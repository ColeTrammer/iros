#include <string.h>

char *strtok_r(char *__restrict str, const char *__restrict delim, char **__restrict save_ptr) {
    char *start = str;
    if (str == NULL) {
        if (save_ptr == NULL || *save_ptr == NULL) {
            return NULL;
        }

        start = *save_ptr;
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

    if (start[i] == '\0') {
        *save_ptr = NULL;
        return start;
    }

end:
    *save_ptr = i == 0 ? NULL : start + i + 1;
    return start;
}