if (Boost_FIND_VERSION VERSION_GREATER "1.62")

add_executable(tests_iperf
    TestIperf.cpp)

add_dependencies(tests_iperf
    nx-iperf)

set_target_properties(tests_iperf
    PROPERTIES
        COMPILE_DEFINITIONS IPERF_BINARY_PATH="$<TARGET_FILE:nx-iperf>")

target_link_libraries(tests_iperf
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES})

add_test(iperf tests_iperf  --log_level=unit_scope)
set_tests_properties(iperf
    PROPERTIES
        TIMEOUT 10)

endif()
