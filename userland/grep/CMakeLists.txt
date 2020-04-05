set(SOURCES
    main.c
)

add_executable(grep ${SOURCES})

install(TARGETS grep RUNTIME DESTINATION bin)