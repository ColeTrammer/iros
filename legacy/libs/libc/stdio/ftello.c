#include <bits/lock.h>
#include <stdio.h>

off_t ftello(FILE *stream) {
    __lock_recursive(&stream->__lock);
    off_t ret = ftello_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
