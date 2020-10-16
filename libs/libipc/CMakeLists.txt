set(SOURCES
    endpoint.cpp
)

add_os_library(libipc ipc TRUE)

target_link_libraries(libipc PUBLIC libeventloop)
