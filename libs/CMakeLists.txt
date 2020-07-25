if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
        libdl
        libm
        libpthread
        librt
    )
endif()

set(TARGETS
    ${OS_LIBS}
    libapp
    libc
    libclanguage
    libclipboard
    libext
    libgraphics
    libliim
    libprocinfo
    libsh
    libws
)

add_all_targets()
set(LIBS_TARGETS ${TARGETS} PARENT_SCOPE)
