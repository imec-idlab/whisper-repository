cmake_minimum_required(VERSION 2.8)
project(rplcapture)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)

if(EXISTS "${PROJECT_SOURCE_DIR}/../analyzer/src")
  set(RPLANALYZER_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/../analyzer/src" CACHE PATH "Path to analyzer source folder")
  if(APPLE)
	set(RPLANALYZER_LIBRARIES "${PROJECT_SOURCE_DIR}/../analyzer/dist/Debug/GNU-Linux-x86/librplanalyzer.dylib" CACHE FILEPATH "Path to the analyzer library folder")
  else()
	set(RPLANALYZER_LIBRARIES "${PROJECT_SOURCE_DIR}/../analyzer/dist/Debug/GNU-Linux-x86/librplanalyzer.so" CACHE FILEPATH "Path to the analyzer library folder")
  endif()
else()
  find_package(RplAnalyzer)
endif()

find_package(PCAP REQUIRED)
find_package(LIBUSB REQUIRED)

message("Using ${RPLANALYZER_LIBRARIES}")

include_directories(${RPLANALYZER_INCLUDE_DIRS} ${PCAP_INCLUDE_DIRS} ${LibUSB_INCLUDE_DIRS})

file(GLOB capture_shared_objects_src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)

add_library(interface_pcap SHARED interface_pcap.c)
target_link_libraries(interface_pcap ${RPLANALYZER_LIBRARIES} pcap pthread)
install(TARGETS interface_pcap DESTINATION lib/rplanalyzer)

add_library(interface_snif SHARED interface_snif.c)
target_link_libraries(interface_snif ${RPLANALYZER_LIBRARIES})
install(TARGETS interface_snif DESTINATION lib/rplanalyzer)

add_library(interface_sensnif SHARED interface_sensnif.c)
target_link_libraries(interface_sensnif ${RPLANALYZER_LIBRARIES})
install(TARGETS interface_sensnif DESTINATION lib/rplanalyzer)

add_library(interface_cc2531 SHARED interface_cc2531.c)
target_link_libraries(interface_cc2531 ${RPLANALYZER_LIBRARIES} pthread ${LibUSB_LIBRARIES})
install(TARGETS interface_cc2531 DESTINATION lib/rplanalyzer)
