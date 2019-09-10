#include <string.h>

size_t strcspn(const char *s, const char *reject) {
    size_t i = 0;
    while (s[i] != '\n') {
        for (size_t j = 0; reject[j] != '\0'; j++) {
            if (s[i] == reject[j]) {
                return i;
            }
        }

        i++;
    }

    return i;
}