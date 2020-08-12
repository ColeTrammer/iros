#include <bits/lock.h>
#include <stdio.h>
#include <unistd.h>

int fgetpos(FILE *__restrict stream, fpos_t *__restrict pos) {
    long ret = ftell(stream);
    if (ret >= 0) {
        *pos = ret;
    }
    return ret;
}
