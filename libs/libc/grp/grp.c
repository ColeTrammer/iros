#include <errno.h>
#include <grp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATIC_GRP_STRING_SIZE 1024

static struct group static_group_buffer;
static char static_group_string_buffer[STATIC_GRP_STRING_SIZE];

struct group *getgrgid(gid_t gid) {
    struct group *result = NULL;
    getgrgid_r(gid, &static_group_buffer, static_group_string_buffer, STATIC_GRP_STRING_SIZE, &result);
    return result;
}

struct group *getgrnam(const char *name) {
    struct group *result = NULL;
    getgrnam_r(name, &static_group_buffer, static_group_string_buffer, STATIC_GRP_STRING_SIZE, &result);
    return result;
}

static void read_gr_entry(char *string, struct group *group) {
    ssize_t i = -1;

#define _(x)        x
#define to_gid_t(x) ((gid_t) strtoul(x, NULL, 10))
#define READ_ENTRY(name, f)                                                     \
    do {                                                                        \
        char *start = &string[++i];                                             \
        for (; string[i] != ':' && string[i] != '\0' && string[i] != '\n'; i++) \
            ;                                                                   \
        string[i] = '\0';                                                       \
        group->gr_##name = f(start);                                            \
    } while (0);

    READ_ENTRY(name, _);
    READ_ENTRY(passwd, _);
    READ_ENTRY(gid, to_gid_t);

    size_t gr_mem_start = ++i;
    size_t count = (string[i] == '\0' || string[i] == '\n') ? 0 : 1;
    for (; string[i] != '\0'; i++) {
        if (string[i] == ',' || string[i] == '\n') {
            string[i] = '\0';
            count++;
        }
    }

    group->gr_mem = (char **) ((((uintptr_t) string + i + sizeof(char *) - 1) / sizeof(char *)) * sizeof(char *));
    for (size_t j = 0; j <= count; j++) {
        if (j == count) {
            group->gr_mem[j] = NULL;
        } else {
            group->gr_mem[j] = string + gr_mem_start;
            while (string[gr_mem_start] != '\0' && string[gr_mem_start] != '\n')
                gr_mem_start++;
        }
    }
}

static int find_gr_entry_impl(struct group *group, char *string_buffer, size_t string_buffer_length,
                              bool (*matches)(struct group *entry, const void *closure), const void *closure) {
    FILE *file = fopen("/etc/group", "r");
    if (file == NULL) {
        return -1;
    }

    char *string = NULL;
    while ((string = fgets(string_buffer, string_buffer_length, file))) {
        read_gr_entry(string, group);

        if (matches(group, closure)) {
            return fclose(file);
        }
    }

    return !fclose(file) ? 1 : -1;
}

static bool gr_matches_name(struct group *entry, const void *closure) {
    return strcmp(entry->gr_name, closure) == 0;
}

static bool gr_matches_gid(struct group *entry, const void *closure) {
    return entry->gr_gid == (gid_t)(uintptr_t) closure;
}

int getgrnam_r(const char *name, struct group *group, char *buf, size_t buflen, struct group **result) {
    int ret = find_gr_entry_impl(group, buf, buflen, gr_matches_name, name);
    if (ret < 0) {
        return ret;
    } else if (ret == 1) {
        *result = NULL;
        errno = ENOENT;
        return -1;
    }

    *result = group;
    return 0;
}

int getgrgid_r(gid_t gid, struct group *group, char *buf, size_t buflen, struct group **result) {
    int ret = find_gr_entry_impl(group, buf, buflen, gr_matches_gid, (const void *) (uintptr_t) gid);
    if (ret < 0) {
        return ret;
    } else if (ret == 1) {
        *result = NULL;
        errno = ENOENT;
        return -1;
    }

    *result = group;
    return 0;
}

static FILE *file = NULL;

void endgrent(void) {
    if (file) {
        fclose(file);
        file = NULL;
    }
}

struct group *getgrent(void) {
    if (!file) {
        file = fopen("/etc/group", "r");
        if (!file) {
            return NULL;
        }
    }

    if (!fgets(static_group_string_buffer, STATIC_GRP_STRING_SIZE, file)) {
        return NULL;
    }
    read_gr_entry(static_group_string_buffer, &static_group_buffer);
    return &static_group_buffer;
}

void setgrent(void) {
    if (file) {
        rewind(file);
    }
}
