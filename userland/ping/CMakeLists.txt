set(SOURCES
    main.c
)

add_executable(ping ${SOURCES})

install(TARGETS ping RUNTIME DESTINATION bin)