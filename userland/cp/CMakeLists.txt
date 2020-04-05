set(SOURCES
    main.c
)

add_executable(cp ${SOURCES})

install(TARGETS cp RUNTIME DESTINATION bin)