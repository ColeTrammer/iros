if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
        libm
        libpthread
        librt
    )
endif()

set(TARGETS
    ${OS_LIBS}
    libc
    libgraphics
    libliim
    libprocinfo
    libsh
    libws
)

add_all_targets()
set(LIBS_TARGETS ${TARGETS} PARENT_SCOPE)
