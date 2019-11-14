#include <stdio.h>
#include <vector.h>

using namespace LIIM;

int main()
{
    Vector<int> list;
    
    for (int i = 0; i < 25; i++) {
        list.add(i);
    }

    for (int i = 0; i < list.size(); i++) {
        printf("%d: %d\n", i, list[i]);
    }

    return 0;
}