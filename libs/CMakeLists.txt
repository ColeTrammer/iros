if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
        libdl
        libm
        libpthread
        librt
        loader
    )
endif()

set(TARGETS
    ${OS_LIBS}
    libapp
    libc
    libclanguage
    libclipboard
    libext
    libeventloop
    libgraphics
    libipc
    libliim
    libprocinfo
    libsh
    libsharedtest
    libsharedtest2
    libws
)

add_all_targets()
set(LIBS_TARGETS ${TARGETS} PARENT_SCOPE)
