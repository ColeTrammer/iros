set(SOURCES
    string/strchr.cpp
)

add_os_library(libccpp ccpp FALSE)
target_include_directories(libccpp PUBLIC include)
target_link_libraries(libccpp libdius)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -nostdinc++")