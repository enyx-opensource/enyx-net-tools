cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Configure project.
project(enyx-net-tools CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

include(GNUInstallDirs)

find_package(Threads REQUIRED)
set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost REQUIRED
    COMPONENTS
        thread system regex program_options date_time
        regex filesystem unit_test_framework)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00) # Windows 10
else()
    add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

