set(SOURCES
    main.c
)

add_executable(kill ${SOURCES})

install(TARGETS kill RUNTIME DESTINATION bin)