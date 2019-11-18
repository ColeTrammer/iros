#ifndef _SYS_UN_H
#define _SYS_UN_H 1

#include <bits/sa_family_t.h>

#define UNIX_PATH_MAX 108

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[UNIX_PATH_MAX];
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_UN_H */