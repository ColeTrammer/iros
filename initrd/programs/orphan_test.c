#include <unistd.h>

int main() {
    for (int i = 0; i < 10; i++) {
        if (fork() == 0) {
            sleep(1);
            return 0;
        }
    }

    return 0;
}
