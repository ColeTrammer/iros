#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(/* int argc, char **argv, char **envp */) {
    for (int i = 0; i < 10; i++) {
        puts(tmpnam(NULL));
    }

    FILE *f = tmpfile();
    assert(f);

    fprintf(f, "hello\n");
    fclose(f);

    return 0;
}
