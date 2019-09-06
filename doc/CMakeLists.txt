find_program(SPHINX_EXECUTABLE NAMES sphinx-build
    DOC "Sphinx documentation generator"
)

if(SPHINX_EXECUTABLE)
    configure_file("conf.py.in" "${CMAKE_CURRENT_BINARY_DIR}/conf.py" @ONLY)
    add_custom_target(enyx-net-tools-doc ALL
        COMMAND
            ${SPHINX_EXECUTABLE}
                -q
                -b html
                -c ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/html
        COMMAND
            ${SPHINX_EXECUTABLE}
                -q
                -b man
                -c ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/man
        COMMENT
            "Building HTML documentation with Sphinx")

    install(
        DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION
            ${CMAKE_INSTALL_DOCDIR}/html
        PATTERN
            .doctrees EXCLUDE
    )

    install(
        DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}/man/
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man1
        FILES_MATCHING
        PATTERN
            "*.1"
        PATTERN
            .doctrees EXCLUDE
    )
endif()
