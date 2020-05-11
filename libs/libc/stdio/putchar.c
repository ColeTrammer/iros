#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int putchar(int c) {
    return fputc(c, stdout);
}

#endif /* OLD_STDIO */
