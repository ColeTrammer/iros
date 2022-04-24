if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
        loader
    )
endif()

set(OS_LIBS
    ${OS_LIBS}
    libliim
)

if (NOT ${MINIMAL_BUILD})
    set(OS_LIBS
        ${OS_LIBS}
        libapp
        libc
        libclanguage
        libclipboard
        libedit
        libeventloop
        libext
        libgraphics
        libgui
        libipc
        libprocinfo
        librepl
        libsh
        libsharedtest
        libsharedtest2
        libterminal
        libtest
        libthread
        libtinput
        libtui
    )
endif()

set(TARGETS ${OS_LIBS})

add_all_targets()
