set(SOURCES
    time/timer_create.cpp
    time/timer_delete.cpp
    time/timer_getoverrun.cpp
    time/timer_gettime.cpp
    time/timer_settime.cpp
)

add_library(rt ${SOURCES})

target_link_libraries(rt PUBLIC pthread)