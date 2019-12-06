#include <liim/linked_list.h>
#include <stdio.h>

int main() {
    LinkedList<int> l;

    for (int i = 0; i < 15; i++) {
        if (i % 3 == 0) {
            l.prepend(i);
        } else {
            l.add(i);
        }
    }

    l.for_each([](auto& elem) {
        printf("%d\n", elem);
    });

    return 0;
}