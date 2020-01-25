#include <liim/vector.h>
#include <stdio.h>

int main() {
	Vector<int> a;
	for (int i = 0; i < 22; i++) {
		a.add(i);
	}

	for (int i = 0; i < a.size(); i++) {
		printf("%d\n", a[i]);
	}

	return 0;
}
