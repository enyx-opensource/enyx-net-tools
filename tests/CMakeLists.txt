add_executable(launcher launcher.cpp)
target_link_libraries(launcher ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(launcher nx-iperf)

macro(iperf_test test_name)
    cmake_parse_arguments(ARG "" "" "A;B" ${ARGN})

    add_test(NAME ${test_name}
             COMMAND launcher "$<TARGET_FILE:nx-iperf>" "${ARG_A}" "${ARG_B}")
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 10 RUN_SERIAL true)
endmacro()

iperf_test(tcp_verify_none
    A "--size=1MiB --verify=none"
    B "--size=1MiB --verify=none")

iperf_test(tcp_verify_first
    A "--size=1MiB --verify=first"
    B "--size=1MiB --verify=first")

iperf_test(tcp_verify_all
    A "--size=1MiB --verify=all"
    B "--size=1MiB --verify=all")

iperf_test(tcp_shutdown_send_complete
    A "--size=1MiB --shutdown-policy=wait_for_peer"
    B "--size=1MiB --shutdown-policy=send_complete")

iperf_test(tcp_shutdown_receive_complete
    A "--size=1MiB --shutdown-policy=wait_for_peer"
    B "--size=1MiB --shutdown-policy=receive_complete")

