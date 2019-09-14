#ifndef _STDARG_H
#define _STDARG_H 1

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef __builtin_va_list va_list;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDARG_H */