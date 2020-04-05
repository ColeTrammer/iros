set(SOURCES
    main.c
)

add_executable(ln ${SOURCES})

install(TARGETS ln RUNTIME DESTINATION bin)