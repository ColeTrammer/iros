set(SOURCES
    main.c
)

add_executable(ps ${SOURCES})
target_link_libraries(ps procinfo)

install(TARGETS ps RUNTIME DESTINATION bin)