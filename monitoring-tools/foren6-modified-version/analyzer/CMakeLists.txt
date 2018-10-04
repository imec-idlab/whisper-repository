cmake_minimum_required(VERSION 2.8)
project(rplanalyzer)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-Wall -std=gnu99)

find_package(PCAP REQUIRED)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../dist/Debug/GNU-Linux-x86/)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../dist/Debug/GNU-Linux-x86/)

add_subdirectory(src)

file(GLOB ANALYZER_CMAKE_MODULE cmake-modules/*.cmake)
install(FILES ${ANALYZER_CMAKE_MODULE} DESTINATION lib/rplanalyzer)
