set(SOURCES
    sync_file.cpp
)

add_os_library(libdius dius TRUE)
target_link_libraries(libdius PUBLIC libdi)
