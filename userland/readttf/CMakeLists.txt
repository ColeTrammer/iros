set(SOURCES
    main.cpp
)

add_os_executable(readttf bin)
target_link_libraries(readttf PRIVATE libgraphics)
