#include <liim/hash_map.h>
#include <liim/string.h>
#include <stdio.h>

int main() {
    String s("asfd");
    puts(s.string());

    HashMap<String, int> map;
    map.put("asdf", 5);
    map.put("aaaa", 10);

    printf("%d\n", *map.get("asdf"));
    printf("%d\n", *map.get("aaaa"));

    puts("");

    return 0;
}
