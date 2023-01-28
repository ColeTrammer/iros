#include <string.h>

// Should respect LC_COLLATE
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
