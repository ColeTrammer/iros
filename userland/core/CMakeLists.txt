cmake_minimum_required(VERSION 3.25.2)

project(Core CXX)

find_package(Dius REQUIRED)

add_executable(ls
    ls.cpp
)

target_link_libraries(ls
  PRIVATE
    Dius::dius
)

install(TARGETS ls
    RUNTIME DESTINATION bin
)
