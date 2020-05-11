#ifndef _STDDEF_H
#define _STDDEF_H 1

#include <bits/null.h>
#include <bits/size_t.h>
#include <bits/wchar_t.h>

#define offsetof __builtin_offsetof

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef long ptrdiff_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDDEF_H */
