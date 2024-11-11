cmake_minimum_required(VERSION 3.21)
set(CMAKE_BUILD_TYPE "Release")
project(xscope_fileio)
add_executable(xscope_host_endpoint xscope_io_host.c)
set_property(TARGET xscope_host_endpoint PROPERTY C_STANDARD 99)
target_include_directories(
  xscope_host_endpoint PRIVATE ../xscope_fileio $ENV{XMOS_TOOL_PATH}/include)
find_library(
  XSCOPE_ENDPOINT_LIB
  NAMES xscope_endpoint.so xscope_endpoint.lib
  PATHS $ENV{XMOS_TOOL_PATH}/lib)
target_link_libraries(xscope_host_endpoint PRIVATE ${XSCOPE_ENDPOINT_LIB})
