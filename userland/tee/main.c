#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-ai] [files...]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    bool ignore_interrupts = false;
    bool append = false;

    int opt;
    while ((opt = getopt(argc, argv, ":ai")) != -1) {
        switch (opt) {
            case 'a':
                append = true;
                break;
            case 'i':
                ignore_interrupts = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (ignore_interrupts) {
        signal(SIGINT, SIG_IGN);
    }

    int open_flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);

    int num_files = argc - optind;
    int *fds = calloc(num_files, sizeof(int));

    int ret = 0;
    for (int i = 0; i < num_files; i++) {
        fds[i] = open(argv[optind + i], open_flags, 0644);
        if (fds[i] == -1) {
            perror("tee: open");
            ret = 1;
            goto done;
        }
    }

    char buf[BUFSIZ];
    for (;;) {
        ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));
        if (nread < 0) {
            perror("tee: read");
            ret = 1;
            goto done;
        }

        if (nread == 0) {
            break;
        }

        for (int i = 0; i < num_files; i++) {
            ssize_t nwritten = write(fds[i], buf, nread);
            if (nwritten < 0) {
                perror("tee: write");
                ret = 1;
                goto done;
            }
        }

        if (write(STDOUT_FILENO, buf, nread) < 0) {
            perror("tee: write");
            ret = 1;
            goto done;
        }
    }

done:
    for (int i = 0; i < num_files; i++) {
        if (fds[i] > 0) {
            if (close(fds[i])) {
                perror("tee: close");
                ret = 1;
            }
        }
    }
    free(fds);
    return ret;
}