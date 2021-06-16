#include <liim/generator.h>
#include <stdio.h>

static Generator<int> do_thing() {
    for (int i = 0; i < 5; i++) {
        co_yield i;
    }
}

int main() {
    auto iter = do_thing();
    while (!iter.finished()) {
        printf("%d\n", iter());
    }
}
