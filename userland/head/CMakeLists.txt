set(SOURCES
    main.c
)

add_executable(head ${SOURCES})

install(TARGETS head RUNTIME DESTINATION bin)