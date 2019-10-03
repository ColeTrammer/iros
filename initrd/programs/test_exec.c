#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** append buffer ***/
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}
void abFree(struct abuf *ab) {
  free(ab->b);
}

int main(/* int argc, char **argv, char **envp */) {
    struct abuf b = ABUF_INIT;
    
    for (size_t i = 0; i <= 4; i++) {
        abAppend(&b, "ABCD", i);
    }

    puts(b.b);

    return 0;
}