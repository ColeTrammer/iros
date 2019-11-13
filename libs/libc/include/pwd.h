#ifndef _PWD_H
#define _PWD_H 1

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct passwd {
    char *pw_name;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_dir;
    char *pw_shell;
};

void endpwend(void);
struct passwd *getpwent(void);
struct passwd *getpwnam(const char *name);
int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result);
struct passwd *getpwuid(uid_t uid);
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result);
void setpwent(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PWD_H */