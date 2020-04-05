set(SOURCES
    main.c
)

add_executable(basename ${SOURCES})

install(TARGETS basename RUNTIME DESTINATION bin)