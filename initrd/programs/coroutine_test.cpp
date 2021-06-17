#include <liim/generator.h>
#include <stdio.h>

static Generator<int> do_thing2() {
    for (int i = 0; i < 3; i++) {
        co_yield i;
    }
    co_return;
}

static Generator<int> do_thing3() {
    co_return;
}

static Generator<int> do_thing() {
    co_yield do_thing3();
    co_yield do_thing2();
    for (int i = 0; i < 5; i++) {
        co_yield i;
    }
    co_return;
}

int main() {
    auto iter = do_thing();
    while (!iter.finished()) {
        printf("%d\n", iter());
    }
}
