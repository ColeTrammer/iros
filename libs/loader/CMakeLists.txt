add_subdirectory("arch/${ARCH}")

set(SOURCES
    ${ARCH_SOURCES}
    dynamic_elf_object.c
    loader.c
    mapped_elf_file.c
    relocations.c
    symbols.c
    tls_record.c
    utils.c
)

add_library(loader SHARED ${SOURCES})
install(TARGETS loader LIBRARY DESTINATION lib)
target_compile_definitions(loader PRIVATE ARCH=${ARCH} __is_libc)
target_compile_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding ${ARCH_CFLAGS})
target_link_options(loader PRIVATE -nostartfiles -nostdlib -ffreestanding --entry=_entry)
set_property(TARGET loader PROPERTY PREFIX "")
