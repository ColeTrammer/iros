set(IRIS_ARCH_CXXFLAGS -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2)
set(IRIS_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/amd64/link.ld)

set(IRIS_ARCH_SOURCES
    arch/x86/amd64/boot/cxx_init.cpp
    arch/x86/amd64/mm/address_space.cpp
    arch/x86/amd64/mm/map_physical_address.cpp
    arch/x86/core/log.cpp
)