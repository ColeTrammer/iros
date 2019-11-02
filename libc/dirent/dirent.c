#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int dirfd(DIR *dir) {
    return dir->fd;
}

int closedir(DIR *dir) {
    int ret = close(dir->fd);
    if (ret < 0) {
        return -1;
    }

    free(dir);
    return 0;
}

DIR *fdopendir(int fd) {
    DIR *dir = calloc(1, sizeof(struct dirent));
    dir->fd = fd;
    return dir;
}

DIR *opendir(const char *path) {
    int fd = open(path, O_DIRECTORY | O_RDONLY, 0);
    if (fd == -1) {
        return NULL;
    }

    return fdopendir(fd);
}

struct dirent *readdir(DIR *dir) {
    ssize_t ret = read(dir->fd, &dir->entry, sizeof(struct dirent));
    if (ret < 0) {
        return NULL;
    }

    return &dir->entry;
}

int alphasort(const struct dirent**a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

static size_t dirent_min_len(struct dirent *d) {
    return sizeof(ino_t) + strlen(d->d_name) + 1;
}

int scandir(const char *dirp, struct dirent ***namelist, int (*filter)(const struct dirent *d), int (*compar)(const struct dirent **a, const struct dirent **b)) {
    DIR *d = opendir(dirp);
    if (d == NULL) {
        return -1;
    }

    *namelist = NULL;
    int count = 0;
    int list_max = 0;
    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (!filter || filter(ent)) {
            if (count >= list_max) {
                list_max += 20;
                *namelist = realloc(*namelist, list_max * sizeof(struct dirent*));
            }

            size_t dirent_len = dirent_min_len(ent);
            struct dirent *to_insert = malloc(dirent_len);
            memcpy(to_insert, ent, dirent_len);
            (*namelist)[count++] = to_insert;
        }
    }

    closedir(d);
    if (compar) {
        qsort(*namelist, count, sizeof(struct dirent*), (int (*)(const void*, const void*)) compar);
    }
    return count;
}