#include <stddef.h>
#include <pwd.h>

void endpwent(void) {

}

struct passwd *getpwent(void) {
    return NULL;
}

struct passwd *getpwnam(const char *name) {
    (void) name;
    return NULL;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    (void) name;
    (void) pwd;
    (void) buf;
    (void) buflen;
    (void) result;
    return -1;
}

static struct passwd pwd = {
    "root", 0, 0, "/", "/bin/sh"
};

struct passwd *getpwuid(uid_t uid) {
    (void) uid;
    return &pwd;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    (void) uid;
    (void) pwd;
    (void) buf;
    (void) buflen;
    (void) result;
    return -1;
}

void setpwent(void) {

}