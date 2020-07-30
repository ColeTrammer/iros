set(SOURCES
    free_procfs_info.c
    read_procfs_global_info.c
    read_procfs_info.c
)

add_os_static_library(libprocinfo procinfo TRUE)
