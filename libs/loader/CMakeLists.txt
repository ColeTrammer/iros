set(SOURCES
    loader.cpp
)

add_library(loader SHARED ${SOURCES})
install(TARGETS loader LIBRARY DESTINATION lib)
target_link_options(loader PRIVATE -nostartfiles -nostdlib)
set_property(TARGET loader PROPERTY PREFIX "")
