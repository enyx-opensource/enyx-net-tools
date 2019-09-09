configure_file("conf.py.in" "${CMAKE_CURRENT_BINARY_DIR}/conf.py" @ONLY)

add_custom_command(
    OUTPUT
        venv/bin/sphinx-build
    COMMAND
        ${Python3_EXECUTABLE} -m venv venv
    COMMAND
        venv/bin/python3 -m pip install --upgrade pip
    COMMAND
        venv/bin/python3 -m pip install sphinx sphinx_rtd_theme
    COMMENT
        "Installing sphinx"
    VERBATIM)

add_custom_target(enyx-net-tools-doc ALL
    DEPENDS
        venv/bin/sphinx-build
    COMMAND
        venv/bin/sphinx-build
            -c ${CMAKE_CURRENT_BINARY_DIR}
            -q
            -b html
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/html
    COMMAND
        venv/bin/sphinx-build
            -c ${CMAKE_CURRENT_BINARY_DIR}
            -q
            -b man
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/man
    COMMENT
        "Building HTML documentation with Sphinx"
    VERBATIM)

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
