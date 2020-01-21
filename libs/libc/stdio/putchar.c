#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

int putchar(int c) {
    return fputc(c, stdout);
}

#endif /* NEW_STDIO */