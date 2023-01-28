#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [user]\n", argv[0]);
        return 2;
    }

    struct passwd *p = NULL;
    if (!argv[1]) {
        p = getpwuid(getuid());
    } else {
        p = getpwnam(argv[1]);
    }

    if (!p) {
        perror("id");
        return 1;
    }

    struct group *g = NULL;
    g = getgrgid(p->pw_gid);
    if (!g) {
        perror("id");
        return 1;
    }

    gid_t group_list[64];
    int num_groups;
    if ((num_groups = getgroups(sizeof(group_list) / sizeof(gid_t), group_list)) < 0) {
        perror("id: getgroups");
        return 1;
    }

    printf("uid=%u(%s) gid=%u(%s) groups=", p->pw_uid, p->pw_name, g->gr_gid, g->gr_name);

    for (int i = 0; i < num_groups; i++) {
        if (i != 0) {
            putchar(',');
        }

        struct group *grp = getgrgid(group_list[i]);
        if (!grp) {
            perror("id");
            return 1;
        }

        printf("%d(%s)", grp->gr_gid, grp->gr_name);
    }

    putchar('\n');
    return 0;
}
