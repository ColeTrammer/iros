#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../builtin.h"

static int op_test(int argc, char **argv) {
    if (argc == 1) {
        return 2;
    }

    if (strcmp(argv[0], "[") == 0) {
        if (strcmp(argv[argc - 1], "]") != 0) {
            fprintf(stderr, "%s: expected `]'\n", argv[0]);
            return 1;
        }

        argc--;
    }

    argc--;
    argv++;

    bool invert = false;
    if (strcmp(argv[0], "!") == 0) {
        invert = true;
        argc--;
        argv++;
    }

    if (argc == 1) {
        return strlen(argv[0]) != 0 ? invert : !invert;
    }

    if (argc == 2) {
        if (strcmp(argv[0], "-b") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISBLK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-c") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISCHR(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-d") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISDIR(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-e") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return invert;
        }
        if (strcmp(argv[0], "-f") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISREG(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-g") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return (st.st_mode & 02000) ? invert : !invert;
        }
        if (strcmp(argv[0], "-h") == 0 || strcmp(argv[0], "-L") == 0) {
            struct stat st;
            if (lstat(argv[1], &st)) {
                return 1;
            }

            return S_ISLNK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-n") == 0) {
            return strlen(argv[1]) != 0 ? invert : !invert;
        }
        if (strcmp(argv[0], "-p") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISFIFO(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-r") == 0) {
            if (access(argv[1], R_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-S") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return S_ISSOCK(st.st_mode) ? invert : !invert;
        }
        if (strcmp(argv[0], "-s") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return st.st_size ? invert : !invert;
        }
        if (strcmp(argv[0], "-t") == 0) {
            return isatty(atoi(argv[1])) ? invert : !invert;
        }
        if (strcmp(argv[0], "-u") == 0) {
            struct stat st;
            if (stat(argv[1], &st)) {
                return 1;
            }

            return (st.st_mode & 04000) ? invert : !invert;
        }
        if (strcmp(argv[0], "-w") == 0) {
            if (access(argv[1], W_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-x") == 0) {
            if (access(argv[1], X_OK)) {
                return !invert;
            }
            return invert;
        }
        if (strcmp(argv[0], "-z") == 0) {
            return strlen(argv[1]) == 0 ? invert : !invert;
        }
    }

    if (argc == 3) {
        if (strcmp(argv[1], "-lt") == 0) {
            return atol(argv[0]) < atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-eq") == 0) {
            return atol(argv[0]) == atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-gt") == 0) {
            return atol(argv[0]) > atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-ge") == 0) {
            return atol(argv[0]) >= atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "-le") == 0) {
            return atol(argv[0]) <= atol(argv[2]) ? invert : !invert;
        }
        if (strcmp(argv[1], "==") == 0) {
            return strcmp(argv[0], argv[2]) == 0 ? invert : !invert;
        }
        if (strcmp(argv[1], "!=") == 0) {
            return strcmp(argv[0], argv[2]) != 0 ? invert : !invert;
        }
    }

    return !invert;
}
SH_REGISTER_BUILTIN([, op_test);
SH_REGISTER_BUILTIN(test, op_test);
