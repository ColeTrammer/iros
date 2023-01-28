set(SOURCES
    main.cpp
    theme_model.cpp
)

add_os_executable(theme_editor bin)
target_link_libraries(theme_editor libgui)
