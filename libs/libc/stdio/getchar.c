#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

int getchar(void) {
    return fgetc(stdin);
}

#endif /* NEW_STDIO */