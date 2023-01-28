#ifndef _WCHAR_H
#define _WCHAR_H 1

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __plusplus */

typedef wchar_t wint_t;
typedef wchar_t wctype_t;

size_t wcslen(const wchar_t *s);
wchar_t *wcscpy(wchar_t *__restrict dest, const wchar_t *__restrict src);
wchar_t *wcschr(const wchar_t *wcs, wchar_t wc);

#ifdef __cplusplus
}
#endif /* __plusplus */

#endif /* _WCHAR_H */
