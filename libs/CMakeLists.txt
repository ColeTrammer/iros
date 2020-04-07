if (NOT ${NATIVE_BUILD})
    set(OS_LIBS
        libc
        libm
        libpthread
        librt
    )
endif()

set(TARGETS
    ${OS_LIBS}
    libgraphics
    libliim
    libprocinfo
    libws
)

add_all_targets()
set(LIBS_TARGETS ${TARGETS} PARENT_SCOPE)