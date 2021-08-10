set(SOURCES
    connection.cpp
)

add_os_library(libclipboard clipboard TRUE)
if (NOT ${NATIVE_BUILD})
    target_link_libraries(libclipboard PRIVATE clipboard_server_headers)
else()
    find_package(X11)
    if (${X11_FOUND})
        target_link_libraries(libclipboard PRIVATE "${X11_LIBRARIES}" pthread)
        target_include_directories(libclipboard PRIVATE "${X11_INCLUDE_DIR}" pthread)
        target_compile_definitions(libclipboard PRIVATE "USE_X11=1")
    endif()
endif()

target_link_libraries(libclipboard PUBLIC libipc)
