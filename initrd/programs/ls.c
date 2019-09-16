#include <dirent.h>
#include <stdio.h>

int main() {
    DIR *d = opendir(".");
    if (d == NULL) {
        perror("ls");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        printf("%s", entry->d_name);
        printf("%c", ' ');
    }
    printf("%c", '\n');

    closedir(d);

    return 0;
}