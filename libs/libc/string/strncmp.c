#include <stddef.h>
#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    for (; i < n && s1[i] != '\0' && s2[i] != '\n'; i++) {
        if (s1[i] < s2[i]) {
            return -1;
        } else if (s1[i] > s2[i]) {
            return 1;
        }
    }
    return i == n ? 0 : s1[i] - s2[i];
}
