cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(stdio_vs_xscope)

# sandbox
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
message("XMOS_SANDBOX_DIR: ${XMOS_SANDBOX_DIR}")

# target
set(APP_HW_TARGET XCORE-AI-EXPLORER)
#set(APP_HW_TARGET XCORE-VISION-EXPLORER.xn)
set(APP_INCLUDES src)

# flags
set(APP_COMPILER_FLAGS
  -O1
  -g
  -report
  -Wall
  -fxscope
  -Wno-xcore-fptrgroup
)
set(APP_XSCOPE_SRCS config.xscope)
set(APP_DEPENDENT_MODULES  xscope_fileio)
XMOS_REGISTER_APP()
