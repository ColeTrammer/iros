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
    libedit
    libeventloop
    libext
    libgraphics
    libipc
    libliim
    libprocinfo
    libsh
    libsharedtest
    libsharedtest2
)

add_all_targets()
