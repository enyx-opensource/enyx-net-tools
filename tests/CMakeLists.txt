if(Boost_VERSION VERSION_GREATER "106200")

add_executable(tests-net-tester
    TestNetTester.cpp)

add_dependencies(tests-net-tester
    enyx-net-tester)

set_target_properties(tests-net-tester
    PROPERTIES
        COMPILE_DEFINITIONS NET_TESTER_BINARY_PATH="$<TARGET_FILE:enyx-net-tester>")

target_link_libraries(tests-net-tester
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES})

add_test(net-tester tests-net-tester  --log_level=unit_scope)
set_tests_properties(net-tester
    PROPERTIES
        TIMEOUT 10)

endif()
