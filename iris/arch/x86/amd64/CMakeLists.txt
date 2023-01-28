target_compile_options(iris
  PRIVATE
    -mno-red-zone
    -mcmodel=kernel
    -mno-mmx
    -mno-sse
    -mno-sse2
)
target_link_options(iris
  PRIVATE
    -T "${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/amd64/link.ld"
    -mno-red-zone
    -mcmodel=kernel
    -mno-mmx
    -mno-sse
    -mno-sse2
)
set_target_properties(iris PROPERTIES
  LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/amd64/link.ld"
)

target_sources(iris
  PRIVATE
    arch/x86/amd64/boot/cxx_init.cpp
    arch/x86/amd64/core/task.cpp
    arch/x86/amd64/mm/address_space.cpp
    arch/x86/amd64/mm/map_physical_address.cpp
    arch/x86/core/log.cpp
)