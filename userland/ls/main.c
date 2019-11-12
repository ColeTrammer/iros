#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdbool.h>

#define LS_STARTING_DIRENTS 100

struct ls_dirent {
    char *name;
    struct stat stat_struct;
};

static struct ls_dirent *dirents = NULL;
static size_t num_dirents = 0;
static size_t num_dirents_max = LS_STARTING_DIRENTS;

static int widest_num_links = 0;
static int widest_size = 0;

static int ls_dirent_compare(const void *a, const void *b) {
    return strcmp(((const struct ls_dirent*) a)->name, ((const struct ls_dirent*) b)->name);
}

void fill_dirent(char *_path, const char *name) {
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return;
    }

    assert(_path);
    assert(name);

    struct ls_dirent d;
    if (_path[0] == '.' && _path[1] == '\0') {
        d.name = strdup(name);
    } else if (strrchr(_path, '/') != _path + strlen(_path) - 1) {
        d.name = malloc(strlen(_path) + strlen(name) + 2);
        strcpy(d.name, _path);
        strcat(d.name, "/");
        strcat(d.name, name);
    } else {
        d.name = malloc(strlen(_path) + strlen(name) + 1);
        strcpy(d.name, _path);
        strcat(d.name, name);
    }

    if (stat(d.name, &d.stat_struct) != 0) {
        perror("ls stat");
        exit(1);
    }

    if (dirents == NULL) {
        dirents = calloc(num_dirents_max, sizeof(struct ls_dirent));
    }

    if (num_dirents >= num_dirents_max) {
        num_dirents_max += LS_STARTING_DIRENTS;
        dirents = realloc(dirents, num_dirents_max * sizeof(struct ls_dirent));
    }

    dirents[num_dirents++] = d;
}

void print_entry(struct ls_dirent *dirent, bool extra_info) {
    if (extra_info) {
        char buffer[50];
        snprintf(buffer, 50, "%%s %%%dd root root %%%dd ", widest_num_links, widest_size);
        
        char perm_string[11];
        perm_string[0] = S_ISDIR(dirent->stat_struct.st_mode) ? 'd' : '-';
        perm_string[1] = dirent->stat_struct.st_mode & S_IRUSR ? 'r' : '-';
        perm_string[2] = dirent->stat_struct.st_mode & S_IWUSR ? 'w' : '-';
        perm_string[3] = dirent->stat_struct.st_mode & S_IXUSR ? 'x' : '-';
        perm_string[4] = dirent->stat_struct.st_mode & S_IRGRP ? 'r' : '-';
        perm_string[5] = dirent->stat_struct.st_mode & S_IWGRP ? 'w' : '-';
        perm_string[6] = dirent->stat_struct.st_mode & S_IXGRP ? 'x' : '-';
        perm_string[7] = dirent->stat_struct.st_mode & S_IROTH ? 'r' : '-';
        perm_string[8] = dirent->stat_struct.st_mode & S_IWOTH ? 'w' : '-';
        perm_string[9] = dirent->stat_struct.st_mode & S_IXOTH ? 'x' : '-';
        perm_string[10] = '\0';

        printf(buffer, perm_string, dirent->stat_struct.st_nlink, dirent->stat_struct.st_size);
    }

    char *color_s = "";
    char *color_end = "";
    char end_char = '\n';
    struct stat *stat_struct = &dirent->stat_struct;
    if (isatty(STDOUT_FILENO)) {
        if (S_ISREG(stat_struct->st_mode)) {
            if (S_IXUSR & stat_struct->st_mode) {
                color_s = "\033[32m";
            } else {
                color_s = "\033[0m";
            }
        } else if (S_ISDIR(stat_struct->st_mode)) {
            color_s = "\033[36m";
        } else if (S_ISCHR(stat_struct->st_mode)) {
            color_s = "\033[35m";
        } else if (S_ISBLK(stat_struct->st_mode)) {
            color_s = "\033[33m";
        } else if (S_ISOCK(stat_struct->st_mode)) {
            color_s = "\033[34m";
        } else {
            color_s = "\033[31m";
        }
        color_end = "\033[0m";
        if (!extra_info) {
            end_char = ' ';
        }
    }

    printf("%s%s%s%c", color_s, dirent->name, color_end, end_char);
}

int main(int argc, char **argv) {
    char *path = ".";
    bool extra_info = false;
    char opt;

    opterr = 0;
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
        case 'l':
            extra_info = true;
            break;
        case '?':
            printf("Usage: %s [path]\n", argv[0]);
            return 0;
        default:
            abort();
        }
    }

    if (optind == argc) {
        argv[argc++] = ".";
    }

    while (optind < argc) {
        path = argv[optind++];

        DIR *d = opendir(path);
        if (d == NULL) {
            struct ls_dirent d = { 0 };
            d.name = strdup(path);
            if (stat(d.name, &d.stat_struct) == -1) {
                perror("ls");
                return 1;
            }

            if (dirents == NULL) {
                dirents = calloc(num_dirents_max, sizeof(struct ls_dirent));
            }

            if (num_dirents >= num_dirents_max) {
                num_dirents_max += LS_STARTING_DIRENTS;
                dirents = realloc(dirents, num_dirents_max * sizeof(struct ls_dirent));
            }

            dirents[num_dirents++] = d;
        } else {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                fill_dirent(path, entry->d_name);
            }
            closedir(d);
        }
    }

    if (extra_info) {
        size_t num_blocks = 0;
        for (size_t i = 0; i < num_dirents; i++) {
            num_blocks += dirents[i].stat_struct.st_blocks;

            char buffer[50];
            memset(buffer, 0, 50);
            snprintf(buffer, 50, "%d", dirents[i].stat_struct.st_nlink);
            widest_num_links = MAX(widest_num_links, strlen(buffer));

            memset(buffer, 0, 50);
            snprintf(buffer, 50, "%lu", dirents[i].stat_struct.st_size);
            widest_size = MAX(widest_size, strlen(buffer));
        }
        printf("total %lu\n", num_blocks);
    }

    qsort(dirents, num_dirents, sizeof(struct ls_dirent), ls_dirent_compare);

    for (size_t i = 0; i < num_dirents; i++) {
        print_entry(dirents + i, extra_info);
    }

    if (!extra_info && isatty(STDOUT_FILENO)) {
        printf("%c", '\n');
    }

    return 0;
}
