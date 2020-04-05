set(SOURCES
    main.c
)

add_executable(initrd ${SOURCES})

target_compile_options(initrd PRIVATE "-Wno-stringop-truncation")

install(TARGETS initrd RUNTIME DESTINATION bin)