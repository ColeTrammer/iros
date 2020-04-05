set(SOURCES
    main.c
)

add_executable(chown ${SOURCES})

install(TARGETS chown RUNTIME DESTINATION bin)