#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int any_failed = 0;

static void test_hosts(void) {
    struct hostent *host;
    sethostent(1);
    while ((host = gethostent())) {
        printf("[host]: %s |%d, %d| (", host->h_name, host->h_addrtype, host->h_length);
        char *iter;
        size_t i = 0;
        while ((iter = host->h_aliases[i++])) {
            printf("%s,", iter);
        }
        printf(") => {");
        i = 0;
        while ((iter = host->h_addr_list[i++])) {
            printf("%s,", inet_ntoa(*(struct in_addr *) iter));
        }
        printf("}\n");
    }
    endhostent();
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-h]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    bool all = true;
    bool test_h = false;

    int opt;
    while ((opt = getopt(argc, argv, ":h")) != -1) {
        switch (opt) {
            case 'h':
                test_h = true;
                all = false;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind != argc) {
        print_usage_and_exit(*argv);
    }

    if (all || test_h) {
        test_hosts();
    }

    return any_failed;
}
