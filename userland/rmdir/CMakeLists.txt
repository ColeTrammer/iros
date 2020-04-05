set(SOURCES
    main.c
)

add_executable(rmdir ${SOURCES})

install(TARGETS rmdir RUNTIME DESTINATION bin)