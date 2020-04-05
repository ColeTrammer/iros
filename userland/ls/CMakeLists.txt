set(SOURCES
    main.c
)

add_executable(ls ${SOURCES})

install(TARGETS ls RUNTIME DESTINATION bin)