if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
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
    librepl
    libsh
    libsharedtest
    libsharedtest2
    libtest
    libthread
    libterminal
    libtinput
    libtui
)

add_all_targets()
