set(SOURCES
    test.c
)

add_os_library(libsharedtest2 sharedtest2 TRUE)
target_link_libraries(libsharedtest2 libsharedtest)
