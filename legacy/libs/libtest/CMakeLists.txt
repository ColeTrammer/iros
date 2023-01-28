set(SOURCES
    test_manager.cpp
)

add_os_library(libtest test TRUE)

target_link_libraries(libtest PUBLIC libliim ${PTHREAD_LIB} libcli)
