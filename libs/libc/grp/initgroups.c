#include <grp.h>

int initgroups(const char *user, gid_t group) {
    gid_t gids[64];
    int num_gids = sizeof(gids) / sizeof(gid_t);
    if (getgrouplist(user, group, gids, &num_gids) < 0) {
        return -1;
    }

    return setgroups(num_gids, gids);
}
