cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(xscope_fileio_tests)
add_subdirectory(no_hang)
add_subdirectory(close_files)
add_subdirectory(simple)
