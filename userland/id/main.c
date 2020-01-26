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

    printf("uid=%u(%s) gid=%u(%s)\n", p->pw_uid, p->pw_name, g->gr_gid, g->gr_name);
    return 0;
}