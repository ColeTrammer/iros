#include <stddef.h>
#include <unistd.h>

int main() {
    for (;;) {
        char c;
        read(0, &c, 1);
        write(1, "MESSAGE\n", 8);
    }
    return 0;
}