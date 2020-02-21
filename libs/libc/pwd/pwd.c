#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATIC_PWD_STRING_SIZE 1024

static struct passwd static_passwd_buffer;
static char static_passwd_string_buffer[STATIC_PWD_STRING_SIZE];

struct passwd *getpwnam(const char *name) {
    struct passwd *result = NULL;
    getpwnam_r(name, &static_passwd_buffer, static_passwd_string_buffer, STATIC_PWD_STRING_SIZE, &result);
    return result;
}

struct passwd *getpwuid(uid_t uid) {
    struct passwd *result = NULL;
    getpwuid_r(uid, &static_passwd_buffer, static_passwd_string_buffer, STATIC_PWD_STRING_SIZE, &result);
    return result;
}

static void read_pw_entry(char *string, struct passwd *passwd) {
    ssize_t i = -1;

#define _(x)        x
#define to_uid_t(x) ((uid_t) strtoul(x, NULL, 10))
#define to_gid_t(x) ((gid_t) strtoul(x, NULL, 10))
#define READ_ENTRY(name, f)                                \
    do {                                                   \
        char *start = &string[++i];                        \
        for (; string[i] != ':' && string[i] != '\0'; i++) \
            ;                                              \
        string[i] = '\0';                                  \
        passwd->pw_##name = f(start);                      \
    } while (0);

    READ_ENTRY(name, _);
    READ_ENTRY(passwd, _);
    READ_ENTRY(uid, to_uid_t);
    READ_ENTRY(gid, to_gid_t);
    READ_ENTRY(gecos, _);
    READ_ENTRY(dir, _);
    READ_ENTRY(shell, _);
}

static int find_pw_entry_impl(struct passwd *passwd, char *string_buffer, size_t string_buffer_length,
                              bool (*matches)(struct passwd *entry, const void *closure), const void *closure) {
    FILE *file = fopen("/etc/passwd", "r");
    if (file == NULL) {
        return -1;
    }

    char *string = NULL;
    while ((string = fgets(string_buffer, string_buffer_length, file))) {
        read_pw_entry(string, passwd);

        if (matches(passwd, closure)) {
            return fclose(file);
        }
    }
    return !fclose(file) ? 1 : -1;
}

static bool pw_matches_name(struct passwd *entry, const void *closure) {
    return strcmp(entry->pw_name, closure) == 0;
}

static bool pw_matches_uid(struct passwd *entry, const void *closure) {
    return entry->pw_uid == (uid_t)(uintptr_t) closure;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    int ret = find_pw_entry_impl(pwd, buf, buflen, pw_matches_name, name);
    if (ret < 0) {
        return ret;
    } else if (ret == 1) {
        *result = NULL;
        errno = ENOENT;
        return -1;
    }

    *result = pwd;
    return 0;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    int ret = find_pw_entry_impl(pwd, buf, buflen, pw_matches_uid, (const void *) (uintptr_t) uid);
    if (ret < 0) {
        return ret;
    } else if (ret == 1) {
        *result = NULL;
        errno = ENOENT;
        return -1;
    }

    *result = pwd;
    return 0;
}

static FILE *file = NULL;

void endpwent(void) {
    if (file) {
        fclose(file);
        file = NULL;
    }
}

struct passwd *getpwent(void) {
    if (!file) {
        file = fopen("/etc/passwd", "r");
        if (!file) {
            return NULL;
        }
    }

    fgets(static_passwd_string_buffer, STATIC_PWD_STRING_SIZE, file);
    read_pw_entry(static_passwd_string_buffer, &static_passwd_buffer);
    return &static_passwd_buffer;
}

void setpwent(void) {
    if (file) {
        rewind(file);
    }
}