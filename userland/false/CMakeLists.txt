set(SOURCES
    main.c
)

add_executable(false ${SOURCES})

install(TARGETS false RUNTIME DESTINATION bin)