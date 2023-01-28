set(ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 -mno-red-zone PARENT_SCOPE)
set(ARCHDIR arch/i686)

set(ARCH_SOURCES
    ../libc/arch/i686/syscall.S
    ${ARCHDIR}/i686_loader.c
    ${ARCHDIR}/i686_relocations.c
    ${ARCHDIR}/plt.S
    PARENT_SCOPE
)
