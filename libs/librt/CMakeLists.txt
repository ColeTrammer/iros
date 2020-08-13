set(SOURCES
    time/timer_create.c
    time/timer_delete.c
    time/timer_getoverrun.c
    time/timer_gettime.c
    time/timer_settime.c
)

add_os_library(librt rt FALSE)
target_link_libraries(librt PUBLIC libpthread)
target_compile_definitions(librt PRIVATE __is_libc)
