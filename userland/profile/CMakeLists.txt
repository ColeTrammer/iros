set(SOURCES
    elf_file.cpp
    main.cpp
    profile.cpp
)

add_os_executable(profile bin)
target_link_libraries(profile libapp libext)
