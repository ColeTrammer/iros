set(SOURCES
    math.c
)

add_os_library(libm m FALSE)
target_compile_definitions(libm PRIVATE __is_libc)

add_os_static_library(libm_static m FALSE)
target_compile_definitions(libm_static PRIVATE __is_libc)
