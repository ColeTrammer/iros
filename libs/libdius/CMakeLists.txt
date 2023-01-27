include(ExternalProject)

set(SOURCES
    linux/io_uring.cpp
    test/test_manager.cpp
    di_assert_impl.cpp
    memory_region.cpp
    sync_file.cpp
)

add_os_library(libdius dius TRUE)
target_link_libraries(libdius PUBLIC libdi)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_ASSERT_HANDLER)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_PLATFORM=<dius/platform.h>)

set(SOURCES test/test_main.cpp)
add_os_library(libdius_test_main dius_test_main FALSE)
target_link_libraries(libdius_test_main PUBLIC libdius)
target_compile_definitions(libdius_test_main PUBLIC DI_CUSTOM_ASSERT_HANDLER)
target_compile_definitions(libdius_test_main PUBLIC DI_CUSTOM_PLATFORM=<dius/platform.h>)