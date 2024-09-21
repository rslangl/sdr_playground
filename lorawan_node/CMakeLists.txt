cmake_minimum_required(VERSION 3.28.0)

set(PROJECT_NAME lorawan_node)

project(
  ${PROJECT_NAME}
  VERSION 0.1.0
  LANGUAGES CXX
)

set(BOARD b_l072z_lrwan1)
set(FLASH_RUNNER openocd)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(
  app
  PRIVATE
  src/main.c
)
