#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool dont_follow_symlinks;
static bool preserve_modifiers;
static bool force;
static bool any_failed;

void do_cp(const char *source_path, const char *dest_path) {
    struct stat st = { 0 };
    int (*do_stat)(const char *path, struct stat *st) = dont_follow_symlinks ? lstat : stat;
    if (do_stat(source_path, &st)) {
        perror("stat");
        any_failed = true;
        return;
    }

    if (S_ISLNK(st.st_mode)) {
        assert(dont_follow_symlinks);
        char *buffer = malloc(st.st_size + 1);
        if (!buffer) {
            perror("malloc");
            any_failed = 1;
            return;
        }

        ssize_t length;
        if ((length = readlink(source_path, buffer, st.st_size)) == -1) {
            perror("readlink");
            any_failed = 1;
            return;
        }

        buffer[length] = '\0';
        if (symlink(buffer, dest_path)) {
            perror("symlink");
            any_failed = 1;
            return;
        }
    } else if (S_ISREG(st.st_mode)) {
        int source = open(source_path, O_RDONLY);
        if (source == -1) {
            perror("open");
            any_failed = true;
            return;
        }

        int dest = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest == -1) {
            if (force) {
                if (unlink(source_path)) {
                    goto open_dest_fail;
                }

                source = open(source_path, O_WRONLY | O_CREAT | O_TRUNC);
                if (source == -1) {
                    goto open_dest_fail;
                }

                goto do_copy;
            }

        open_dest_fail:
            perror("open");
            any_failed = true;
            if (close(source)) {
                perror("close");
            }
            return;
        }

    do_copy : {}
        char buf[BUFSIZ];
        ssize_t len;
        while ((len = read(source, buf, BUFSIZ)) > 0) {
            if (write(dest, buf, len) != len) {
                perror("write");
                any_failed = true;
                break;
            }
        }

        if (close(source) | close(dest)) {
            perror("close");
            any_failed = true;
        }
    } else {
        fprintf(stderr, "cp: `%s' is not a regular file\n", source_path);
        any_failed = 1;
        return;
    }

    if (preserve_modifiers) {
        struct timespec tms[2] = { st.st_atim, st.st_mtim };
        if (utimensat(AT_FDCWD, dest_path, tms, dont_follow_symlinks ? AT_SYMLINK_NOFOLLOW : 0)) {
            perror("utimesat");
            any_failed = 1;
        }

        int (*do_chown)(const char *path, uid_t owner, gid_t group) = dont_follow_symlinks ? lchown : chown;

        if (do_chown(dest_path, st.st_uid, st.st_gid)) {
            perror("chown");
            any_failed = 1;
        }

        if (!S_ISLNK(st.st_mode)) {
            if (chmod(dest_path, st.st_mode)) {
                perror("chmod");
                any_failed = 1;
            }
        }
    }
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-Pfp] <source...> <target>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":Pfp")) != -1) {
        switch (opt) {
            case 'P':
                dont_follow_symlinks = true;
                break;
            case 'f':
                force = true;
                break;
            case 'p':
                preserve_modifiers = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (argc - optind < 2) {
        print_usage_and_exit(*argv);
    }

    const char *target = argv[argc - 1];
    struct stat st;

    bool target_exists = true;
    if (stat(target, &st)) {
        target_exists = false;
        if (errno != ENOENT) {
            perror("stat");
            return 1;
        }
    }

    bool target_is_dir = target_exists && S_ISDIR(st.st_mode);
    bool target_must_be_dir = argc - optind > 2;
    if (target_must_be_dir && !target_is_dir) {
        fprintf(stderr, "cp: %s is not a directory\n", target);
        return 1;
    }

    for (; optind < argc - 1; optind++) {
        if (target_is_dir) {
            char *path = malloc(strlen(argv[optind] + strlen(target) + 2));
            if (!path) {
                perror("malloc");
                return 1;
            }
            stpcpy(stpcpy(stpcpy(path, target), "/"), argv[optind]);
            do_cp(argv[optind], path);
            free(path);
        } else {
            do_cp(argv[optind], target);
        }
    }

    return any_failed ? 1 : 0;
}