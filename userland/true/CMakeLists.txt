set(SOURCES
    main.c
)

add_executable(true ${SOURCES})

install(TARGETS true RUNTIME DESTINATION bin)