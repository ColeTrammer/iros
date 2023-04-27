find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    include(ExternalProject)
    ExternalProject_Add(
        doxygen_awesome_css
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_SHALLOW TRUE
        GIT_TAG main
        TMP_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css/tmp"
        STAMP_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css/stamp"
        DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css"
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css/src"
        BINARY_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css/src"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL 1
    )
    set(DOXYGEN_AWESOME_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/doxygen-awesome-css/src")

    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_XML NO)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_DOT_TRANSPARENT YES)
    set(DOXYGEN_USE_MATHJAX YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_COLLABORATION_GRAPH NO)
    set(DOXYGEN_INCLUDE_GRAPH NO)
    set(DOXYGEN_INCLUDED_BY_GRAPH NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/docs/mainpage.md")
    set(DOXYGEN_LAYOUT_FILE "${CMAKE_SOURCE_DIR}/docs/DoxygenLayout.xml")
    set(DOXYGEN_IMAGE_PATH "${CMAKE_SOURCE_DIR}/docs/assets")
    set(DOXYGEN_EXCLUDE_PATTERNS "*/tests/*")
    set(DOXYGEN_PREDEFINED __CCPP_BEGIN_DECLARATIONS= __CCPP_END_DECLARATIONS= __CCPP_RESTRICT=restrict)
    set(DOXYGEN_STRIP_FROM_INC_PATH "${CMAKE_SOURCE_DIR}/iris/include" "${CMAKE_SOURCE_DIR}/libs/ccpp/include"
                                    "${CMAKE_SOURCE_DIR}/libs/di/include" "${CMAKE_SOURCE_DIR}/libs/dius/include"
    )
    set(DOXYGEN_HTML_COLORSTYLE "LIGHT")
    set(DOXYGEN_HTML_HEADER "${CMAKE_SOURCE_DIR}/docs/header.html")
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${DOXYGEN_AWESOME_DIR}/doxygen-awesome.css")
    set(DOXYGEN_HTML_EXTRA_FILES
        "${DOXYGEN_AWESOME_DIR}/doxygen-awesome-darkmode-toggle.js"
        "${DOXYGEN_AWESOME_DIR}/doxygen-awesome-fragment-copy-button.js"
        "${DOXYGEN_AWESOME_DIR}/doxygen-awesome-paragraph-link.js"
        "${DOXYGEN_AWESOME_DIR}/doxygen-awesome-interactive-toc.js"
        "${DOXYGEN_AWESOME_DIR}/doxygen-awesome-tabs.js"
    )

    doxygen_add_docs(
        docs ${CMAKE_SOURCE_DIR}/libs ${CMAKE_SOURCE_DIR}/iris ${CMAKE_SOURCE_DIR}/userland ${CMAKE_SOURCE_DIR}/docs
    )

    add_dependencies(docs doxygen_awesome_css)
endif()
