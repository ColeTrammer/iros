set(SOURCES
    connection.cpp
)

add_os_library(libclipboard clipboard TRUE)
if (NOT ${NATIVE_BUILD})
    target_link_libraries(libclipboard PRIVATE clipboard_server_headers)
else()
    target_link_libraries(libclipboard PRIVATE X11 pthread)
endif()

target_link_libraries(libclipboard PUBLIC libliim)
