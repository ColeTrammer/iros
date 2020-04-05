set(SOURCES
    main.cpp
)

add_executable(yes ${SOURCES})

target_link_libraries(yes liim)
install(TARGETS yes RUNTIME DESTINATION bin)