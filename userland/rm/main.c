#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static bool recursive;
static bool interactive;
static bool suppress;
static bool input_is_terminal;
static bool failed;

#define rm_err(s)        \
    do {                 \
        if (!suppress) { \
            perror(s);   \
        }                \
    } while (0)

static bool should_prompt(bool can_write) {
    return (!suppress && !can_write && input_is_terminal) || interactive;
}

enum prompt_result { PROMPT_ERR, PROMPT_YES, PROMPT_NO };

static enum prompt_result do_prompt(const char *path) {
    fprintf(stderr, "Do you really want to remove: `%s'? (y/n) ", path);

    char buf[1024];
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        if (ferror(stdin)) {
            rm_err("fgets");
            return PROMPT_ERR;
        }
        return PROMPT_NO;
    }

    if (strcasecmp(buf, "y\n") == 0 || strcasecmp(buf, "yes\n") == 0) {
        return PROMPT_YES;
    }
    return PROMPT_NO;
}

static int do_remove_with_info(const char *path, const struct stat *st) {
    bool is_link = S_ISLNK(st->st_mode);
    bool can_write = is_link ? true : access(path, W_OK) == 0;
    if (!can_write && errno != EACCES) {
        rm_err("access");
        return 1;
    }

    if (should_prompt(can_write)) {
        enum prompt_result result = do_prompt(path);
        switch (result) {
            case PROMPT_NO:
                return 0;
            case PROMPT_YES:
                break;
            case PROMPT_ERR:
                return 1;
        }
    }

    // NOTE: is seems like POSIX would allow calling rm <dir> with no flags, but linux does't.
    int (*remover)(const char *path) = S_ISDIR(st->st_mode) ? rmdir : unlink;
    if (remover(path)) {
        rm_err(S_ISDIR(st->st_mode) ? "rmdir" : "unlink");
        return 1;
    }

    return 0;
}

static int do_remove(char *path) {
    struct stat st;
    if (lstat(path, &st)) {
        rm_err("lstat");
        return 1;
    }

    return do_remove_with_info(path, &st);
}

static int rm_recurse(const char *path, const struct stat *st, int flag, struct FTW *ftwbuf) {
    (void) flag;
    (void) ftwbuf;

    if (do_remove_with_info(path, st)) {
        failed = true;
    }
    return 0;
}

static int do_rm(char *path) {
    if (!recursive) {
        return do_remove(path);
    }

    if (nftw(path, rm_recurse, 10, FTW_PHYS | FTW_DEPTH)) {
        rm_err("ntfw");
        return 1;
    }

    return 0;
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-fiRr] <file...>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    input_is_terminal = isatty(STDIN_FILENO) ? true : false;

    int opt;
    while ((opt = getopt(argc, argv, ":friR")) != -1) {
        switch (opt) {
            case 'r':
            case 'R':
                recursive = true;
                break;
            case 'i':
                interactive = true;
                break;
            case 'f':
                interactive = false;
                suppress = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (optind == argc) {
        if (suppress) {
            return 0;
        }
        print_usage_and_exit(*argv);
    }

    bool any_failed = false;
    for (; optind < argc; optind++) {
        if (do_rm(argv[optind])) {
            any_failed = true;
        }
    }

    // This global is a hack we can't return values through ntfw without causing the
    // file traversal to stop
    if (failed) {
        any_failed = true;
    }

    return any_failed && !suppress ? 1 : 0;
}