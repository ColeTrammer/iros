set(UCD_VERSION "14.0.0")

set(UCD_DOWNLOAD_FILES
    "extracted/DerivedEastAsianWidth.txt"
)

foreach(FILE ${UCD_DOWNLOAD_FILES})
    string(REPLACE "/" "-" SAFE_FILE ${FILE})
    add_custom_command(
        OUTPUT "${FILE}"
        COMMAND curl -sLo "${FILE}" "https://www.unicode.org/Public/${UCD_VERSION}/ucd/${FILE}"
        VERBATIM
    )
    add_custom_target("download-${SAFE_FILE}" DEPENDS "${FILE}")
endforeach()

set(GENERATED_FILES
    unicode/east_asian_width.cpp
    unicode/east_asian_width_data.h
    unicode/east_asian_width.h
)

add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND ${NATIVE_UNICODE} -o "${CMAKE_CURRENT_BINARY_DIR}/unicode" -d "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
    DEPENDS ${NATIVE_UNICODE} ${UCD_DOWNLOAD_FILES}
)
add_custom_target(generate-unicode-files DEPENDS ${GENERATED_FILES})

set(SOURCES
    ${GENERATED_FILES}
    generic_sh_parser.cpp
    sh_lexer.cpp
)


set(SOURCES
    ${GENERATED_FILES}
    property_range.cpp
    terminal_width.cpp
)

add_os_library(libunicode unicode true)

target_include_directories(libunicode PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(libunicode libliim)
