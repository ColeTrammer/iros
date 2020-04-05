set(SOURCES
    main.c
)

add_executable(chmod ${SOURCES})

install(TARGETS chmod RUNTIME DESTINATION bin)