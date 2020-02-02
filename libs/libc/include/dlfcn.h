#ifndef _DLFCN_H
#define _DLFCN_H 1

#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_GLOBAL 4
#define RTLD_LOCAL  8

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int dlclose(void *handle);
char *dlerror(void);
void *dlopen(const char *path, int flags);
void *dlsym(void *__restrict handle, const char *__restrict symbol);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DLFCN_H */