#ifndef _EXECINFO_H
#define _EXECINFO_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int backtrace(void **buffer, int buffer_max);
void dump_backtrace(void *const *buffer, int buffer_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EXECINFO_H */
