set(SOURCES
    loader.c
    malloc.c
    plt.S
    printf.c
    utils.c
)

add_library(loader SHARED ${SOURCES})
install(TARGETS loader LIBRARY DESTINATION lib)
target_compile_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding -mno-mmx -mno-sse -mno-sse2)
target_link_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding --entry=_entry)
set_property(TARGET loader PROPERTY PREFIX "")
