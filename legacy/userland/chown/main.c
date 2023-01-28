#include <errno.h>
#include <ftw.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *UID_GID_SCANF_STRING = sizeof(uid_t) == 2 ? "%hd" : "%d";

static char *prog_name;
static bool chgrp;
static bool any_failed;
static bool recursive;
static bool follow_symlinks = true;

static uid_t uid = -1;
static gid_t gid = -1;

static void do_chown(const char *path) {
    int (*chown_function)(const char *path, uid_t uid, gid_t gid) = follow_symlinks ? chown : lchown;
    if (chown_function(path, uid, gid)) {
        fprintf(stderr, "%s: chown failed for `%s': %s\n", prog_name, path, strerror(errno));
        any_failed = 1;
    }
}

static int recursive_chown(const char *path, const struct stat *st, int typeflag, struct FTW *ftwbuf) {
    (void) st;
    (void) typeflag;
    (void) ftwbuf;
    do_chown(path);
    return 0;
}

void print_usage_and_exit(const char *s) {
    if (chgrp) {
        printf("Usage: %s [-hR] <group> <file...>\n", s);
    } else {
        printf("Usage: %s [-hR] <user:group> <file...>\n", s);
    }
    exit(2);
}

int main(int argc, char **argv) {
    prog_name = *argv;
    char *last_slash = strrchr(prog_name, '/');
    if (!last_slash) {
        last_slash = prog_name - 1;
    }
    chgrp = strcmp(last_slash + 1, "chgrp") == 0;

    int opt;
    while ((opt = getopt(argc, argv, ":hR")) != -1) {
        switch (opt) {
            case 'h':
                follow_symlinks = false;
                break;
            case 'R':
                recursive = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind < 2) {
        print_usage_and_exit(*argv);
    }

    char *user;
    char *group;
    if (chgrp) {
        user = NULL;
        group = argv[optind++];
    } else {
        user = argv[optind++];
        group = strchr(user, ':');
        if (group) {
            *group++ = '\0';
        }
    }

    if (user) {
        struct passwd *p = getpwnam(user);
        if (!p) {
            if (sscanf(user, UID_GID_SCANF_STRING, &uid) != 1) {
                fprintf(stderr, "%s: invalid user: %s\n", prog_name, user);
            }
        } else {
            uid = p->pw_uid;
        }
    }

    if (group) {
        struct group *g = getgrnam(group);
        if (!g) {
            if (sscanf(group, UID_GID_SCANF_STRING, &gid) != 1) {
                fprintf(stderr, "%s: invalid group: %s\n", prog_name, group);
            }
        } else {
            gid = g->gr_gid;
        }
    }

    for (; optind < argc; optind++) {
        if (recursive) {
            nftw(argv[optind], recursive_chown, FOPEN_MAX - 3, FTW_PHYS | FTW_DEPTH);
        } else {
            do_chown(argv[optind]);
        }
    }

    return any_failed;
}
