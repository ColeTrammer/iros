if (NOT ${NATIVE_BUILD})
    add_subdirectory("arch/${ARCH}")

    add_library(boot_startfiles OBJECT
        arch/${ARCH}/boot.S
        arch/${ARCH}/crti.S
        arch/${ARCH}/crtn.S
    )

    set(SOURCES
        ${BOOT_ARCH_SOURCES}
    )

    add_executable(boot ${SOURCES})

    target_compile_options(boot PRIVATE -ffreestanding)
    target_link_options(boot PRIVATE -nostdlib ${BOOT_ARCH_LDFLAGS})

    add_dependencies(boot boot_startfiles)

    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtend.o OUTPUT_VARIABLE CRTEND_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(STARTFILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/boot_startfiles.dir/arch/${ARCH})
    set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_COMPILER} <CMAKE_C_LINK_FLAGS> <FLAGS> <LINK_FLAGS> ${STARTFILE_PREFIX}/boot.S.o ${STARTFILE_PREFIX}/crti.S.o ${CRTBEGIN_PATH} <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CRTEND_PATH} ${STARTFILE_PREFIX}/crtn.S.o")

    install(TARGETS boot RUNTIME DESTINATION boot)

    set(BOOT_TARGETS boot PARENT_SCOPE)
endif()