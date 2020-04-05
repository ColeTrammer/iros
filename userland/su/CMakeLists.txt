set(SOURCES
    main.c
)

add_executable(su ${SOURCES})

install(TARGETS su RUNTIME DESTINATION bin)