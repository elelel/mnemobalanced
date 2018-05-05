cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(mnemobalanced LANGUAGES CXX)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(SOURCES
  ${SRC}/dictionary.cpp
  ${SRC}/solver.cpp
  ${SRC}/mb.cpp)

add_executable(mb ${SOURCES})

set_target_properties(mb PROPERTIES
  CXX_STANDARD 17
  )



