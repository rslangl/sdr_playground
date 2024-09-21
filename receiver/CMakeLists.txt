cmake_minimum_required(VERSION 3.28.0)

project(
  sdr-playground
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(
  sdr_playground
  main.cpp
)

target_link_libraries(
  sdr_playground
  PRIVATE
  SoapySDR
  fftw3
)
