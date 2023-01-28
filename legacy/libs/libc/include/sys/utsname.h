#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct utsname {
    char sysname[20];
    char nodename[20];
    char release[20];
    char version[20];
    char machine[20];
};

int uname(struct utsname *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_UTSNAME_H */
