#ifndef _STDDEF_H
#define _STDDEF_H 1

#define NULL ((void*) 0)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef long ptrdiff_t;

typedef char wchar_t;

typedef unsigned long size_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDDEF_H */