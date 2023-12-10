target_compile_options(iris PRIVATE -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2)
target_link_options(
    iris
    PRIVATE
    -T
    "${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/amd64/link.ld"
    -mno-red-zone
    -mcmodel=kernel
    -mno-mmx
    -mno-sse
    -mno-sse2
)
set_target_properties(iris PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/amd64/link.ld")
