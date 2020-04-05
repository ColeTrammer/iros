set(SOURCES
    main.c
)

add_executable(clear ${SOURCES})

install(TARGETS clear RUNTIME DESTINATION bin)