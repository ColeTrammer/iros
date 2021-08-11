set(SOURCES
    application.cpp
    flex_layout_engine.cpp
    frame.cpp
    panel.cpp
)

add_os_library(libtui tui true)

target_link_libraries(libtui libtinput libeventloop)
