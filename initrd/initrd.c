#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_NAME_LENGTH 64

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
            n++;
        }
        closedir(d);
        return n - 2;
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s dir outfile\n", argv[0]);
        return 1;
    }

    int64_t files = count_files(argv[1]);
    if (files <= 0) {
        printf("No Files in Dir: %s\n", argv[1]);
        return 1;
    }
    printf("Process %ld file(s)...\n", files);

    size_t dir_name_length = strlen(argv[1]) + 1;
    char *file_name = calloc(dir_name_length + MAX_FILE_NAME_LENGTH, 1);
    strcpy(file_name, argv[1]);
    file_name[dir_name_length - 1] = '/';

    FILE *initrd = fopen(argv[2], "w");
    if (!initrd) {
        printf("Invalid outfile: %s\n", argv[2]);
        return 1;
    }

    fwrite(&files, sizeof(int64_t), 1, initrd);

    size_t *file_lengths = calloc(files, sizeof(size_t));

    DIR *d = opendir(argv[1]);
    if (d) {
        struct initrd_file_entry entry;
        uint32_t offset = sizeof(int64_t) + files * sizeof(struct initrd_file_entry);

        struct dirent *dir;
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
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

    d = opendir(argv[1]);
    if (d) {
        struct dirent *dir;
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
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