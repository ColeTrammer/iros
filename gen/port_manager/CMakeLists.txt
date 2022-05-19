set(SOURCES
    config.cpp
    context.cpp
    json_reader.cpp
    main.cpp
    port.cpp
    step.cpp
)

add_os_executable(port_manager bin)
target_link_libraries(port_manager PRIVATE libcli libliim libext)
target_compile_definitions(port_manager PRIVATE "IROS_ROOT=\"${ROOT}\"" "IROS_ARCH=\"${ARCH}\"")
