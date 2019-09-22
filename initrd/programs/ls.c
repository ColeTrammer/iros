#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

void print_entry(char *_path, struct dirent *dirent) {
    char *path = malloc(strlen(_path) + strlen(dirent->d_name) + 2);
    strcpy(path, _path);
    strcat(path, "/");
    strcat(path, dirent->d_name);

    struct stat *stat_struct = malloc(sizeof(struct stat));
    stat(path, stat_struct);
    if (stat_struct == NULL) {
        free(path);
        exit(1);
    }

    char *color_s;
    if (S_ISREG(stat_struct->st_mode)) {
        color_s = "\033[0m";
    } else if (S_ISDIR(stat_struct->st_mode)) {
        color_s = "\033[36m";
    } else if (S_ISCHR(stat_struct->st_mode)) {
        color_s = "\033[35m";
    } else if (S_ISBLK(stat_struct->st_mode)) {
        color_s = "\033[32m";
    } else {
        color_s = "\033[31m";
    }

    printf("%s%s\033[0m", color_s, dirent->d_name);
    free(path);
}

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
        print_entry(path, entry);
        printf("%c", ' ');
    }
    printf("%c", '\n');

    closedir(d);

    return 0;
}