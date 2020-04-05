set(SOURCES
    main.c
)

add_executable(start ${SOURCES})

install(TARGETS start RUNTIME DESTINATION bin)