set(SOURCES
    di_assert_impl.cpp
    sync_file.cpp
)

add_os_library(libdius dius TRUE)
target_link_libraries(libdius PUBLIC libdi)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_ASSERT_HANDLER)