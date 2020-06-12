#include <grp.h>
#include <stdbool.h>
#include <string.h>

int getgrouplist(const char *user, gid_t extra_group, gid_t *groups, int *ngroups) {
    int max_groups = *ngroups;
    int groups_found = 0;
    bool found_extra_group = false;
    struct group *grp;

    setgrent();
    while ((grp = getgrent())) {
        bool in_group = false;
        for (size_t i = 0; grp->gr_mem[i]; i++) {
            const char *group_member = grp->gr_mem[i];
            if (strcmp(user, group_member) == 0) {
                in_group = true;
                break;
            }
        }

        if (in_group) {
            if (grp->gr_gid == extra_group) {
                found_extra_group = true;
            }

            if (groups_found < max_groups) {
                groups[groups_found] = grp->gr_gid;
            }
            groups_found++;
        }
    }
    endgrent();

    if (!found_extra_group) {
        if (groups_found < max_groups) {
            groups[groups_found] = extra_group;
        }
        groups_found++;
    }

    *ngroups = groups_found;
    return max_groups <= groups_found ? groups_found : -1;
}
