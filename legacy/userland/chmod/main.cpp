#include <errno.h>
#include <ext/parse_mode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static Ext::Mode mode;
static bool any_failed;
static bool verbose;
static const char *program_name;

void do_chmod(const char *path) {
    struct stat st;
    if (stat(path, &st)) {
        fprintf(stderr, "%s: failed to stat `%s': %s\n", program_name, path, strerror(errno));
        any_failed = 1;
        return;
    }

    mode_t new_mode = mode.resolve(st.st_mode);
    new_mode &= 07777;

    if (verbose) {
        fprintf(stderr, "chmod \"%s\" from \"%#.5o\" to \"%#.5o\"\n", path, st.st_mode & 07777, new_mode);
    }

    if (chmod(path, new_mode)) {
        fprintf(stderr, "%s: failed to chmod `%s': %s\n", program_name, path, strerror(errno));
    }
}

void print_usage_and_exit(const char *s) {
    printf("Usage: %s [-v] <mode> <file...>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    program_name = *argv;

    int opt;
    while ((opt = getopt(argc, argv, ":v")) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
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

    String mode_string = argv[optind++];
    auto maybe_mode = Ext::parse_mode(mode_string);
    if (!maybe_mode.has_value()) {
        fprintf(stderr, "%s: failed to parse mode `%s'\n", *argv, mode_string.string());
        return 1;
    }
    mode = maybe_mode.value();

    for (; optind < argc; optind++) {
        do_chmod(argv[optind]);
    }

    return !!any_failed;
}
