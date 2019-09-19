#include <stdlib.h>
#include <ctype.h>

int atoi(const char *s) {
	int n = 0;
    size_t i = 0;
    int sign = 1;

    while (isspace(s[i])) { i++; }

    if (s[i] == '+' || s[i] == '-') {
        sign = s[i] == '-' ? -1 : 1;
        i++;
    }

	for (; s[i] != '\0' && isdigit(s[i]); i++) {
		int digit = s[i] - '0';
		n *= 10;
		n += digit;
	}

	return sign * n;
}