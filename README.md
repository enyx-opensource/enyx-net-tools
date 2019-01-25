[![Build Status](https://travis-ci.org/enyx-opensource/net-tools.svg?branch=master)](https://travis-ci.org/enyx-opensource/net-tools)
[![Build status](https://ci.appveyor.com/api/projects/status/h84jq9hmg3iglqpo?svg=true)](https://ci.appveyor.com/project/DavidKeller/net-tools)


Description
===========
This package contains various tools used
to generate and verify network traffic.

Build
=====

Requirements
------------
* CMake
* boost devel

Steps
------------
On linux/BSD:

    $mkdir build && cd build
    $cmake ..
    $make -j`nproc`

On Windows:

    >mkdir build && cd build
    >cmake -G "Visual Studio 14 2015 Win64"
    >cmake --build . --config Release

Usage
=====

nx-iperf
--------
Wait for a tcp connection on _loopback_ port _12345_ and generate _25Mb/s_ traffic:

    $nx-iperf --listen 127.0.0.1:12345 --tx-bandwidth 25Mb --size 1Gb

Connect on _loopback_ port _12345_ and generate _10Mb/s_ traffic while checking received data:

    $nx-iperf --connect 127.0.0.1:12345 --tx-bandwidth 10Mb --verify=all --size 256Mb

Connect on _loopback_ port _12345_ and generate _10Mb/s_ traffic while capping reception to _5Mb/s_:

    $nx-iperf --connect 127.0.0.1:12345 --tx-bandwidth 10Mb --rx-bandwidth 5Mb

Connect on remote _192.168.10.4_ port _12345_ from local address _192.168.10.3_ port _54321_ and generate _10Mb/s_ traffic while capping reception to _5Mb/s_:

    $nx-iperf --connect 192.168.10.3:54321:192.168.10.4:12345 --tx-bandwidth 10Mb --rx-bandwidth 5Mb

