set(SOURCES
    main.c
)

add_executable(uname ${SOURCES})

install(TARGETS uname RUNTIME DESTINATION bin)