#include <dirent.h>
#include <inttypes.h>
#include <regex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILE_NAME_LENGTH 64

static char *exclude;

struct initrd_file_entry {
    char name[MAX_FILE_NAME_LENGTH];
    uint32_t offset;
    uint32_t length;
} __attribute__((packed));

int64_t count_files(char *dir) {
    DIR *d = opendir(dir);
    if (d) {
        int64_t n = 0;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            struct stat st;
            if (fstatat(dirfd(d), dir->d_name, &st, 0)) {
                perror("fstatat");
                return -1;
            }

            if (S_ISREG(st.st_mode) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..") &&
                (!exclude || strcmp(dir->d_name, exclude))) {
                n++;
            }
        }
        closedir(d);
        return n;
    }
    return -1;
}

void print_usage_and_exit(const char *name) {
    fprintf(stderr, "Usage: %s [-e exclude-file] <dir> <output>\n", name);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, ":e:")) != -1) {
        switch (opt) {
            case 'e':
                exclude = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind < 2) {
        print_usage_and_exit(*argv);
    }

    char *dir = argv[optind];
    char *output = argv[optind + 1];

    int64_t files = count_files(dir);
    if (files <= 0) {
        printf("No Files in Dir: %s\n", dir);
        return 1;
    }
    printf("Process %" PRIi64 " file(s)...\n", files);

    size_t dir_name_length = strlen(dir) + 1;
    char *file_name = calloc(dir_name_length + MAX_FILE_NAME_LENGTH, 1);
    strcpy(file_name, dir);
    file_name[dir_name_length - 1] = '/';

    FILE *initrd = fopen(output, "w");
    if (!initrd) {
        printf("Invalid outfile: %s\n", output);
        return 1;
    }

    fwrite(&files, sizeof(int64_t), 1, initrd);

    size_t *file_lengths = calloc(files, sizeof(size_t));

    DIR *d = opendir(dir);
    if (d) {
        struct initrd_file_entry entry;
        uint32_t offset = sizeof(int64_t) + files * sizeof(struct initrd_file_entry);

        struct dirent *dir;
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            struct stat st;
            if (fstatat(dirfd(d), dir->d_name, &st, 0)) {
                perror("fstatat");
                return 1;
            }

            if (S_ISREG(st.st_mode) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..") &&
                (!exclude || strcmp(dir->d_name, exclude))) {
                if (strlen(dir->d_name) > MAX_FILE_NAME_LENGTH) {
                    printf("File Name Too Long: %s\n", dir->d_name);
                    return 1;
                }
                printf("File: %s\n", dir->d_name);

                strncpy(entry.name, dir->d_name, sizeof(entry.name));
                memset(entry.name + strlen(dir->d_name), '\0', MAX_FILE_NAME_LENGTH - strlen(dir->d_name));

                strncpy(file_name + dir_name_length, dir->d_name, MAX_FILE_NAME_LENGTH);
                memset(file_name + dir_name_length + strlen(dir->d_name), '\0', MAX_FILE_NAME_LENGTH - strlen(dir->d_name));

                FILE *current_file = fopen(file_name, "r");
                if (!current_file) {
                    printf("Error reading file: %s\n", file_name);
                    return 1;
                }

                fseek(current_file, 0, SEEK_END);
                entry.offset = offset;
                entry.length = ftell(current_file);
                file_lengths[i++] = entry.length;
                entry.length = (entry.length & ~7) + 8;
                offset += entry.length;
                fclose(current_file);

                fwrite(&entry, sizeof(struct initrd_file_entry), 1, initrd);
            }
        }

        closedir(d);
    }

    d = opendir(dir);
    if (d) {
        struct dirent *dir;
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            struct stat st;
            if (fstatat(dirfd(d), dir->d_name, &st, 0)) {
                perror("fstatat");
                return 1;
            }

            if (S_ISREG(st.st_mode) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..") &&
                (!exclude || strcmp(dir->d_name, exclude))) {
                size_t buffer_length = (file_lengths[i] & ~7) + 8;
                uint8_t *buffer = calloc(buffer_length, sizeof(uint8_t));

                strncpy(file_name + dir_name_length, dir->d_name, MAX_FILE_NAME_LENGTH);
                memset(file_name + dir_name_length + strlen(dir->d_name), '\0', MAX_FILE_NAME_LENGTH - strlen(dir->d_name));

                FILE *current_file = fopen(file_name, "r");
                size_t read = fread(buffer, 1, file_lengths[i], current_file);
                if (read < file_lengths[i++]) {
                    printf("Error reading file: %s\n", file_name);
                    return 1;
                }
                fwrite(buffer, 1, buffer_length, initrd);
                fclose(current_file);

                free(buffer);
            }
        }

        closedir(d);
    }

    fclose(initrd);
    free(file_name);

    return 0;
}
