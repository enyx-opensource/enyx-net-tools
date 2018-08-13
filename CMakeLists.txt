cmake_minimum_required(VERSION 2.8.4)

# Configure project.
project(enyx-net-tools)

find_package(Threads REQUIRED)
set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.40 REQUIRED
    COMPONENTS thread system regex program_options date_time regex)

if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00) # Windows 10
endif ()

add_definitions(-DBOOST_FOREACH_NO_RVALUE_DETECTION=1)

set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

include_directories(${Boost_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIR})

add_subdirectory(src)
