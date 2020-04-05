set(SOURCES
    main.c
)

add_executable(tail ${SOURCES})

install(TARGETS tail RUNTIME DESTINATION bin)