#include <grp.h>

extern struct group __static_group_buffer;
extern char __static_group_string_buffer[1024];

struct group *getgrgid(gid_t gid) {
    struct group *result;
    getgrgid_r(gid, &__static_group_buffer, __static_group_string_buffer, sizeof(__static_group_string_buffer), &result);
    return result;
}
