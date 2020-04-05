set(SOURCES
    main.c
)

add_executable(stat ${SOURCES})

install(TARGETS stat RUNTIME DESTINATION bin)