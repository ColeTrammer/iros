set(SOURCES
    main.c
)

add_executable(rm ${SOURCES})

install(TARGETS rm RUNTIME DESTINATION bin)