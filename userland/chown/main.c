#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_usage_and_exit(char **argv) {
    printf("Usage: %s <user:group> <file-name>\n", argv[0]);
    exit(2);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage_and_exit(argv);
    }

    char *user = argv[1];
    char *group = strchr(user, ':');
    if (!group) {
        print_usage_and_exit(argv);
    }

    *group++ = '\0';

    uid_t uid;
    struct passwd *p = getpwnam(user);
    if (!p) {
        if (sscanf(user, "%hd", &uid) != 1) {
            print_usage_and_exit(argv);
        }
    } else {
        uid = p->pw_uid;
    }

    gid_t gid;
    struct group *g = getgrnam(group);
    if (!g) {
        if (sscanf(group, "%hd", &gid) != 1) {
            print_usage_and_exit(argv);
        }
    } else {
        gid = g->gr_gid;
    }

    if (chown(argv[2], uid, gid)) {
        perror("chown");
        return 1;
    }

    return 0;
}
