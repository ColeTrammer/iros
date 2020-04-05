set(SOURCES
    main.c
)

add_executable(top ${SOURCES})
target_link_libraries(top procinfo)

install(TARGETS top RUNTIME DESTINATION bin)