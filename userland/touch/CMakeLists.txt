set(SOURCES
    main.c
)

add_executable(touch ${SOURCES})

install(TARGETS touch RUNTIME DESTINATION bin)