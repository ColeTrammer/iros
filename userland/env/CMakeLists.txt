set(SOURCES
    main.c
)

add_executable(env ${SOURCES})

install(TARGETS env RUNTIME DESTINATION bin)