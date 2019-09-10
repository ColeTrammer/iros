#include <string.h>
#include <stdbool.h>

/* Uses naive approach for simplicity, but should be changed later */
char *strstr(const char *s, const char *sub) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        bool matches = true;
        for (size_t j = 0; sub[j] != '\0'; j++) {
            if (s[i + j] == '\0') {
                return NULL;
            } else if (s[i + j] != sub[j]) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return (char*) s + i;
        }
    }

    return NULL;
}