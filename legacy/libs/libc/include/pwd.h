#ifndef _PWD_H
#define _PWD_H 1

#include <bits/gid_t.h>
#include <bits/size_t.h>
#include <bits/uid_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct passwd {
    char *pw_name;
    char *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

int getpwent_r(struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp);
int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result);
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result);

struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PWD_H */
