set(SOURCES
    main.c
)

add_executable(date ${SOURCES})

install(TARGETS date RUNTIME DESTINATION bin)