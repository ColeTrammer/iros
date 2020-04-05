set(SOURCES
    main.c
)

add_executable(cat ${SOURCES})

install(TARGETS cat RUNTIME DESTINATION bin)