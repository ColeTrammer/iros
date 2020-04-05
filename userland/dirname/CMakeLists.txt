set(SOURCES
    main.c
)

add_executable(dirname ${SOURCES})

install(TARGETS dirname RUNTIME DESTINATION bin)