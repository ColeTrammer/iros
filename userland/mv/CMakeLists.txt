set(SOURCES
    main.c
)

add_executable(mv ${SOURCES})

install(TARGETS mv RUNTIME DESTINATION bin)