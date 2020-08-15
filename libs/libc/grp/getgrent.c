#include <grp.h>

struct group __static_group_buffer = { 0 };
char __static_group_string_buffer[1024] = { 0 };

struct group *getgrent(void) {
    struct group *ret;
    getgrent_r(&__static_group_buffer, __static_group_string_buffer, sizeof(__static_group_string_buffer), &ret);
    return ret;
}
