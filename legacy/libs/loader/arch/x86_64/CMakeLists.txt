set(ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)
set(ARCHDIR arch/x86_64)

set(ARCH_SOURCES
    ${ARCHDIR}/arch_loader.c
    ${ARCHDIR}/arch_relocations.c
    ${ARCHDIR}/plt.S
    PARENT_SCOPE
)
