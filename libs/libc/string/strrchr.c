#include <string.h>
#include <stdbool.h>

char *strrchr(const char *s, int c) {
    char val = (char) c;

    size_t last_occurence = 0;
    bool found = false;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == val) {
            last_occurence = i;
            found = true;
        }
    }

    return found ? (char*) s + last_occurence : NULL;
}