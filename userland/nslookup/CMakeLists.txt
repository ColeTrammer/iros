set(SOURCES
    dns.c
    main.c
    mapping.c
    server.c
)

add_executable(nslookup ${SOURCES})

install(TARGETS nslookup RUNTIME DESTINATION bin)