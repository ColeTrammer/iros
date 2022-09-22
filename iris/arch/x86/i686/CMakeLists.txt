set(IRIS_ARCH_CXXFLAGS -mno-mmx -mno-sse -mno-sse2 -mno-red-zone)
set(IRIS_ARCH_LDFLAGS -T ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/i686/link.ld)

set(IRIS_ARCH_SOURCES
)