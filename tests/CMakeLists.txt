add_executable(tests_iperf
    TestIperf.cpp)

set_target_properties(tests_iperf
    PROPERTIES
        COMPILE_DEFINITIONS IPERF_BINARY_PATH="$<TARGET_FILE:nx-iperf>")

target_link_libraries(tests_iperf
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES})

add_test(iperf tests_iperf)
set_tests_properties(iperf
    PROPERTIES
        TIMEOUT 10)

