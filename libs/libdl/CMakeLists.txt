set(SOURCES
    dl_error_storage.cpp
    dlclose.cpp
    dlerror.cpp
    dlopen.cpp
    dlsym.cpp
)

add_os_library(libdl dl FALSE)

target_link_libraries(libdl PRIVATE libext)
