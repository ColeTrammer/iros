set(SOURCES
    main.cpp
)

add_os_executable(cpa bin)
target_link_libraries(cpa PRIVATE libdius)
