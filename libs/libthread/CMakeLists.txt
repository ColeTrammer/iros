set(SOURCES
    background_job.cpp
)

add_os_library(libthread thread TRUE)

target_link_libraries(libthread PUBLIC libeventloop ${PTHREAD_LIB})
