#include <new>
#include <stdio.h>
#include <vector.h>

using namespace LIIM;

int main()
{
    Vector<Vector<int>> ll;
    fprintf(stderr, "Constructor succeeded\n");
    for (int i = 0; i < 25; i++) {
        ll.add(Vector<int>());
        for (int j = 0; j < 25; j++) {
            ll[i].add(j);
        }
    }

    for (int i = 0; i < 25; i++) {
        fprintf(stderr, "ll[%d][%d] == %d\n", i, i, ll[i][i]);
    }

    return 0;
}