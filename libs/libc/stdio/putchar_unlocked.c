#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int putchar_unlocked(int c) {
    return fputc_unlocked(c, stdout);
}

#endif /* OLD_STDIO */
