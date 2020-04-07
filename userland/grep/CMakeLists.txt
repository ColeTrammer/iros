set(SOURCES
    main.c
)

add_os_executable(grep bin)

if (${NATIVE_BUILD})
    target_compile_definitions(grep PRIVATE USERLAND_NATIVE)
    target_link_libraries(grep PRIVATE libregex)
endif()