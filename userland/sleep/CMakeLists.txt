set(SOURCES
    main.c
)

add_executable(sleep ${SOURCES})

install(TARGETS sleep RUNTIME DESTINATION bin)