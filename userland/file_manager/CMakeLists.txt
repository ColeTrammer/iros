set(SOURCES
    file_system_model.cpp
    main.cpp
)

add_os_executable(file_manager bin)
target_link_libraries(file_manager libapp libext libtui)
