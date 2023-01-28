#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main() {
    for (;;) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        int ret = select(1, &set, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            return 1;
        }

        fputs("done\n", stderr);

        assert(ret == 1);
        assert(FD_ISSET(STDIN_FILENO, &set));

        char c;
        assert(read(STDIN_FILENO, &c, 1) == 1);
        assert(write(STDOUT_FILENO, &c, 1) == 1);
    }

    return 0;
}
