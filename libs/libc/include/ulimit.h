#ifndef _ULIMIT_H
#define _ULIMIT_H 1

#define UL_GETFSIZE 0
#define UL_SETFSIZE 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

long ulimit(int, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ULIMIT_H */
