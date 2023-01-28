set(SOURCES
    main.c
)

add_os_executable(initrd bin)

target_compile_options(initrd PRIVATE "-Wno-stringop-truncation")
