set(SOURCES
    math.c
)

add_library(m ${SOURCES})

install(TARGETS m ARCHIVE DESTINATION usr/lib)