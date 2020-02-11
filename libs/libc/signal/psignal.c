#include <signal.h>
#include <stdio.h>
#include <string.h>

void psignal(int sig, const char *s) {
    if (s && s[0] != '\0') {
        fprintf(stderr, "%s: ", s);
    }

    fputs(strsignal(sig), stderr);
}