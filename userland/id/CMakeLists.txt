set(SOURCES
    main.c
)

add_executable(id ${SOURCES})

install(TARGETS id RUNTIME DESTINATION bin)