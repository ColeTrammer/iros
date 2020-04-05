set(SOURCES
    main.c
)

add_executable(realpath ${SOURCES})

install(TARGETS realpath RUNTIME DESTINATION bin)