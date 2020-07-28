set(SOURCES
    loader.cpp
    malloc.c
    printf.c
    utils.c
)

add_library(loader SHARED ${SOURCES})
install(TARGETS loader LIBRARY DESTINATION lib)
target_compile_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding)
target_link_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding --entry=_entry)
set_property(TARGET loader PROPERTY PREFIX "")
