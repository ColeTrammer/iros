#ifndef _DLFCN_H
#define _DLFCN_H 1

#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_GLOBAL 4
#define RTLD_LOCAL  8

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    const char *dli_fname;
    void *dli_fbase;
    const char *dli_sname;
    void *dli_saddr;
} Dl_info;

int dladdr(void *addr, Dl_info *info);
int dlclose(void *handle);
char *dlerror(void);
void *dlopen(const char *path, int flags);
void *dlsym(void *__restrict handle, const char *__restrict symbol);

#ifdef __libc_internal
extern __attribute__((weak)) int __dl_has_error;
extern __attribute__((weak)) char __dl_error[256];

#define __dl_set_error(...)                                        \
    do {                                                           \
        snprintf(__dl_error, sizeof(__dl_error) - 1, __VA_ARGS__); \
        __dl_has_error = 1;                                        \
    } while (0)

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DLFCN_H */
