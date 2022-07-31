set(SOURCES
    main.cpp
)

add_os_executable(unicode bin)
target_link_libraries(unicode PUBLIC libcli libliim libext)
target_compile_definitions(unicode PRIVATE "IROS_ROOT=\"${ROOT}\"" "IROS_ARCH=\"${ARCH}\"")
