#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static bool dont_follow_symlinks;
static bool preserve_modifiers;
static bool force;
static bool interactive;
static bool recursive;
static bool verbose;

#define cp_log(m, ...)                                     \
    do {                                                   \
        if (verbose) {                                     \
            fprintf(stderr, m __VA_OPT__(, ) __VA_ARGS__); \
        }                                                  \
    } while (0)

static bool any_failed;
static bool target_is_dir;

static const char *target;

enum prompt_result { PROMPT_ERR, PROMPT_YES, PROMPT_NO };

static enum prompt_result do_prompt(const char *path) {
    fprintf(stderr, "Do you really want to overwrite: `%s'? (y/n) ", path);

    char buf[1024];
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        if (ferror(stdin)) {
            perror("fgets");
            return PROMPT_ERR;
        }
        return PROMPT_NO;
    }

    if (strcasecmp(buf, "y\n") == 0 || strcasecmp(buf, "yes\n") == 0) {
        return PROMPT_YES;
    }
    return PROMPT_NO;
}

void do_preserve_modifiers(const struct stat *st, const char *dest_path) {
    if (preserve_modifiers) {
        int (*do_chown)(const char *path, uid_t owner, gid_t group) = dont_follow_symlinks ? lchown : chown;

        if (do_chown(dest_path, st->st_uid, st->st_gid)) {
            perror("chown");
            any_failed = 1;
        }

        if (!S_ISLNK(st->st_mode)) {
            if (chmod(dest_path, st->st_mode)) {
                perror("chmod");
                any_failed = 1;
            }
        }

        struct timespec tms[2] = { st->st_atim, st->st_mtim };
        if (utimensat(AT_FDCWD, dest_path, tms, dont_follow_symlinks ? AT_SYMLINK_NOFOLLOW : 0)) {
            perror("utimesat");
            any_failed = 1;
        }
    }
}

void do_cp(const char *source_path, const char *dest_path) {
    cp_log("do_cp(\"%s\", \"%s\")\n", source_path, dest_path);

    struct stat st = { 0 };
    int (*do_stat)(const char *path, struct stat *st) = dont_follow_symlinks ? lstat : stat;
    if (do_stat(source_path, &st)) {
        perror("stat");
        any_failed = true;
        return;
    }

    if (interactive) {
        if (access(dest_path, F_OK) == 0) {
            enum prompt_result result = do_prompt(dest_path);
            switch (result) {
                case PROMPT_ERR:
                    any_failed = 1;
                    return;
                case PROMPT_NO:
                    return;
                case PROMPT_YES:
                    break;
            }
        }
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

                source = open(source_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

    do_preserve_modifiers(&st, dest_path);
}

const char *path_base_at_level(const char *path, int level) {
    char *last_slash = NULL;
    do {
        char *last_slash_save = last_slash;
        if (last_slash) {
            *last_slash = '\0';
        }
        last_slash = strrchr(path, '/');
        if (last_slash_save) {
            *last_slash_save = '/';
        }

        if (!last_slash) {
            return path;
        }
    } while (level--);

    return last_slash + 1;
}

static int do_cp_r(const char *source, const struct stat *st, int type, struct FTW *ftwbuf) {
    const char *relative_name = path_base_at_level(source, ftwbuf->level - !target_is_dir);
    char *dest_path = malloc(strlen(target) + strlen(relative_name) + 2);
    stpcpy(stpcpy(stpcpy(dest_path, target), "/"), relative_name);

    if (type == FTW_D) {
        const char *dir_path = (ftwbuf->level == 0 && !target_is_dir) ? target : dest_path;
        cp_log("mkdir(\"%s\")\n", dir_path);
        if (mkdir(dir_path, 0777)) {
            perror("mkdir");
            any_failed = true;
        }

        do_preserve_modifiers(st, dir_path);
    } else {
        do_cp(source, dest_path);
    }

    free(dest_path);
    return 0;

    return 0;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-rR] [-Pfipv] <source...> <target>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":rRPfipv")) != -1) {
        switch (opt) {
            case 'P':
                dont_follow_symlinks = true;
                break;
            case 'f':
                force = true;
                break;
            case 'i':
                interactive = true;
                break;
            case 'p':
                preserve_modifiers = true;
                break;
            case 'r':
            case 'R':
                recursive = true;
                break;
            case 'v':
                verbose = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (argc - optind < 2) {
        print_usage_and_exit(*argv);
    }

    target = argv[argc - 1];
    struct stat st;

    bool target_exists = true;
    if (stat(target, &st)) {
        target_exists = false;
        if (errno != ENOENT) {
            perror("stat");
            return 1;
        }
    }

    target_is_dir = target_exists && S_ISDIR(st.st_mode);
    bool target_must_be_dir = (argc - optind > 2);
    if (target_must_be_dir && !target_is_dir) {
        fprintf(stderr, "cp: %s is not a directory\n", target);
        return 1;
    }

    int cp_r_flags = FTW_PHYS;
    for (; optind < argc - 1; optind++) {
        if (recursive) {
            if (nftw(argv[optind], do_cp_r, 10, cp_r_flags) < 0) {
                perror("nftw");
            }
        } else if (target_is_dir) {
            char *name = basename(argv[optind]);
            char *path = malloc(strlen(name + strlen(target) + 2));
            if (!path) {
                perror("malloc");
                return 1;
            }
            stpcpy(stpcpy(stpcpy(path, target), "/"), name);
            do_cp(argv[optind], path);
            free(path);
        } else {
            do_cp(argv[optind], target);
        }
    }

    return any_failed ? 1 : 0;
}
