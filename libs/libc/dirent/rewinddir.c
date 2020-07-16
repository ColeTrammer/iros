#include <dirent.h>
#include <unistd.h>

void rewinddir(DIR *d) {
    lseek(d->fd, 0, SEEK_SET);
}
