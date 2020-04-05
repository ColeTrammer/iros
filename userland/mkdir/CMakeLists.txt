set(SOURCES
    main.c
)

add_executable(mkdir ${SOURCES})

install(TARGETS mkdir RUNTIME DESTINATION bin)