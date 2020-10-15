set(SOURCES
)

add_os_library(libipc app TRUE)

target_link_libraries(libipc PUBLIC libapp)
