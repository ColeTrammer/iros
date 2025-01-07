cmake_minimum_required(VERSION 3.25.2)

project(AudioTest CXX)

find_package(Dius REQUIRED)
find_package(Diusaudio REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(audio_test_headers INTERFACE)
add_library(AudioTest::audio_test_headers ALIAS audio_test_headers)

target_sources(audio_test_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(audio_test_headers INTERFACE Dius::dius Di::di)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

add_executable(audio_test ${sources})
target_link_libraries(audio_test PUBLIC Dius::dius Diusaudio::diusaudio Di::di)

install(TARGETS audio_test RUNTIME DESTINATION bin)
