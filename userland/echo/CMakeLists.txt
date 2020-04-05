set(SOURCES
    main.c
)

add_executable(echo ${SOURCES})

install(TARGETS echo RUNTIME DESTINATION bin)