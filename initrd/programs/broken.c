#include <stddef.h>

int main() {
    *((int*) NULL) = 0;
    return 1;
}