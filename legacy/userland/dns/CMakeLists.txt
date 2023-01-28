add_os_headers(dns_service_headers)

set(SOURCES
    dns.c
    main.c
    mapping.c
    server.c
)

add_os_executable(dns bin)
target_link_libraries(dns dns_service_headers)
