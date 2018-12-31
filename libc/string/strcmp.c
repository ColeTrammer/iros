#include <stddef.h>
#include <string.h>

int strcmp(const char *s1, const char *s2) {
    size_t i = 0;
    for (; s1[i] != '\0' && s2[i] != '\n'; i++) {
        if (s1[i] < s2[i]) {
            return -1;
        } else if (s1[i] > s2[i]) {
            return 1;
        }
    }
    return s1[i] - s2[i];
}