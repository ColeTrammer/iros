set(ARCH_CFLAGS PARENT_SCOPE)
set(KERNEL_ARCH_CFLAGS -mno-mmx -mno-sse -mno-sse2 PARENT_SCOPE)

set(ARCHDIR arch/i686)

set(ARCH_FREE_SOURCES PARENT_SCOPE)
set(ARCH_HOSTED_SOURCES
    ${ARCHDIR}/bits/___tls_get_addr.c
    ${ARCHDIR}/bits/__get_self.c
    ${ARCHDIR}/execinfo/backtrace.c
    ${ARCHDIR}/setjmp/_longjmp.S
    ${ARCHDIR}/setjmp/_setjmp.S
    ${ARCHDIR}/setjmp/longjmp.S
    ${ARCHDIR}/setjmp/setjmp.S
    ${ARCHDIR}/setjmp/sigsetjmp.S
    ${ARCHDIR}/crt0.S
    ${ARCHDIR}/sigreturn.S
    ${ARCHDIR}/syscall.S
    PARENT_SCOPE
)
