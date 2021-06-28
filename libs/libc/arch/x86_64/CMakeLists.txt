set(ARCH_CFLAGS PARENT_SCOPE)
set(KERNEL_ARCH_CFLAGS -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)

set(ARCHDIR arch/x86_64)

set(ARCH_FREE_SOURCES PARENT_SCOPE)
set(ARCH_HOSTED_SOURCES
    ${ARCHDIR}/bits/__get_self.c
    ${ARCHDIR}/bits/__tls_get_addr.c
    ${ARCHDIR}/execinfo/backtrace.c
    ${ARCHDIR}/setjmp/_longjmp.S
    ${ARCHDIR}/setjmp/_setjmp.S
    ${ARCHDIR}/setjmp/longjmp.S
    ${ARCHDIR}/setjmp/setjmp.S
    ${ARCHDIR}/setjmp/sigsetjmp.S
    ${ARCHDIR}/crt0.S
    ${ARCHDIR}/exit.c
    ${ARCHDIR}/sigreturn.S
    ${ARCHDIR}/syscall.S
    PARENT_SCOPE
)
