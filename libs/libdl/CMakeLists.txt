set(SOURCES
    dlclose.cpp
    dlerror.cpp
    dlopen.cpp
    dlsym.cpp
)

add_os_library(libdl dl FALSE)
