#include <liim/inline_queue.h>
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

    struct ListItem : public InlineLinkedListNode<ListItem> {
        ListItem(int _value) { value = _value; }

        int value;
    };

    InlineQueue<ListItem> items;
    items.add({ 0 });
    items.add({ 1 });
    items.add({ 2 });
    assert(items.size() == 3);

    printf("Head: %p\n", items.head());

    while (!items.empty()) {
        int value = items.take_one().value;
        printf("Value: %d\n", value);
    }

    return 0;
}
