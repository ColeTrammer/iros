set(SOURCES
    2022/01/solution.cpp
    2022/02/solution.cpp
    2022/20/solution.cpp
    main.cpp
)

add_os_executable(aoc bin)
target_link_libraries(aoc libdi libdius)