#include <stddef.h>
#include <unistd.h>

int main() {
    *((int*) NULL) = 0;
    return 0;
}