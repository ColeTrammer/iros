set(ARCHDIR arch/x86_64)

set(BOOT_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld PARENT_SCOPE)

set(BOOT_ARCH_SOURCES
    ${ARCHDIR}/cpu_check.S
    ${ARCHDIR}/screen.S
    ${ARCHDIR}/load_kernel.c  
    PARENT_SCOPE
)