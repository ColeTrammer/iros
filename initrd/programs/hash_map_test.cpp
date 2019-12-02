#include <liim/hash_map.h>
#include <stdio.h>

int main() {
    HashMap<int, const char*> map;

    map.put(0, "First");
    map.put(1, "Second");
    map.put(2, "Third");

    puts(*map.get(0));
    map.remove(1);
    assert(map.get(1) == nullptr);
    puts(*map.get(2));
}