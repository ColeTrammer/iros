#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

int main(int argc, char **argv) {
    bool print_sysname = argc == 1;
    bool print_nodename = false;
    bool print_release = false;
    bool print_version = false;
    bool print_machine = false;
    bool print_all = false;

    char opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "asnrvm")) != -1) {
        switch (opt) {
            case 'a':
                print_all = true;
                break;
            case 's':
                print_sysname = true;
                break;
            case 'n':
                print_nodename = true;
                break;
            case 'r':
                print_release = true;
                break;
            case 'v':
                print_version = true;
                break;
            case 'm':
                print_machine = true;
                break;
            case '?':
                printf("Usage: %s [-snrvm]\n", argv[0]);
                return 0;
            default:
                abort();
        }
    }

    struct utsname u;
    if (uname(&u) < 0) {
        perror("uname");
        return 1;
    }

    bool did_something = false;

    if (print_all || print_sysname) {
        printf("%s%s", did_something ? " " : "", u.sysname);
        did_something = true;
    }

    if (print_all || print_nodename) {
        printf("%s%s", did_something ? " " : "", u.nodename);
        did_something = true;
    }

    if (print_all || print_version) {
        printf("%s%s", did_something ? " " : "", u.version);
        did_something = true;
    }

    if (print_all || print_release) {
        printf("%s%s", did_something ? " " : "", u.release);
        did_something = true;
    }

    if (print_all || print_machine) {
        printf("%s%s", did_something ? " " : "", u.machine);
        did_something = true;
    }

    if (did_something) {
        printf("%c", '\n');
    }

    return 0;
}
