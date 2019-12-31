#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [epoch timespamp]\n", argv[0]);
        return 0;
    }

    time_t _time_;
    if (argc == 1) {
        _time_ = time(NULL);
    } else {
        _time_ = atol(argv[1]);
    }

    printf("%s", ctime(&_time_));

    return 0;
}