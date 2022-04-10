set(ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)
set(ARCHDIR arch/i686)

set(ARCH_SOURCES
    ${ARCHDIR}/i686_loader.c
    ${ARCHDIR}/i686_relocations.c
    PARENT_SCOPE
)
