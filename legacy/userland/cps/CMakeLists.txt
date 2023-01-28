set(SOURCES
    main.cpp
)

add_os_executable(cps bin)
target_link_libraries(cps PRIVATE libdius)