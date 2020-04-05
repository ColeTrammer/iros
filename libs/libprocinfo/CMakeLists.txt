set(SOURCES
    free_procfs_info.c
    read_procfs_global_info.c
    read_procfs_info.c
)

add_library(procinfo ${SOURCES})

target_include_directories(procinfo PUBLIC include)