#include <dirent.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char *path;
    if (argc == 1) {
        path = ".";
    } else if (argc == 2) {
        path = argv[1];
    } else {
        printf("Usage: %s [path]\n", argv[0]);
        return 0;
    }

    DIR *d = opendir(path);
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