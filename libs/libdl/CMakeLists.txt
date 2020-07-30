set(SOURCES
    dl_error_storage.c
    dlclose.c
    dlerror.c
    dlopen.c
    dlsym.c
)

add_os_library(libdl dl FALSE)
