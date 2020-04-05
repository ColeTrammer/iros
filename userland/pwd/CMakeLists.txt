set(SOURCES
    main.c
)

add_executable(pwd ${SOURCES})

install(TARGETS pwd RUNTIME DESTINATION bin)