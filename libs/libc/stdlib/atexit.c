#include <bits/cxx.h>
#include <stdlib.h>

int atexit(void (*f)(void)) {
    return __cxa_atexit((void (*)(void *)) f, NULL, NULL);
}
