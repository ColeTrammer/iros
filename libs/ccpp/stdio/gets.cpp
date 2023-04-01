#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/gets.html
extern "C" char* gets(char* str) {
    auto guard = di::ScopedLock(stdin->locked.get_lock());
    auto* current = str;
    for (;;) {
        auto ch = fgetc_unlocked(stdin);
        if (ch == EOF) {
            return nullptr;
        }
        if (ch == '\n') {
            break;
        }
        *current++ = char(ch);
    }
    *current = '\0';
    return str;
}
