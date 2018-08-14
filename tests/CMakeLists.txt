add_executable(launcher launcher.cpp)
target_link_libraries(launcher ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(launcher nx-iperf)

macro(iperf_test test_name)
    cmake_parse_arguments(ARG "" "" "SERVER;CLIENT" ${ARGN})

    add_test(NAME ${test_name}
        COMMAND launcher "$<TARGET_FILE:nx-iperf>" "${ARG_SERVER}" "${ARG_CLIENT}")
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 10 RUN_SERIAL true)
endmacro()

iperf_test(tcp_verify_none
    SERVER "--size=1MiB --verify=none --duration-margin=01:00"
    CLIENT "--size=1MiB --verify=none --duration-margin=01:00")

iperf_test(tcp_verify_first
    SERVER "--size=1MiB --verify=first --duration-margin=01:00"
    CLIENT "--size=1MiB --verify=first --duration-margin=01:00")

iperf_test(tcp_verify_all
    SERVER "--size=1MiB --verify=all --duration-margin=01:00"
    CLIENT "--size=1MiB --verify=all --duration-margin=01:00")

