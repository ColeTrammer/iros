include(ExternalProject)

set(SOURCES
    linux/io_uring.cpp
    di_assert_impl.cpp
    memory_region.cpp
    sync_file.cpp
)

add_os_library(libdius dius TRUE)
target_link_libraries(libdius PUBLIC libdi)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_ASSERT_HANDLER)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_PLATFORM=<dius/platform.h>)