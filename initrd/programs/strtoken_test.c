#include <stdio.h>
#include <string.h>

int main() {
    char str[80] = "echo asdf  asdf\n";
    const char *s = " \r\t\n\a";
    char *token;

    /* get the first token */
    token = strtok(str, s);

    /* walk through other tokens */
    while (token != NULL) {
        printf(" %s\n", token);

        token = strtok(NULL, s);
    }

    return (0);
}
