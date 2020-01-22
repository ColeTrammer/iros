#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int getchar(void) {
    return fgetc(stdin);
}

#endif /* OLD_STDIO */