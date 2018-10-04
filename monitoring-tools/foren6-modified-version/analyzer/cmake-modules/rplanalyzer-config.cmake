# - Find RplAnalyzer library
#
#  This module defines the following variables:
#     RPLANALYZER_FOUND       - True if RPLANALYZER_INCLUDE_DIR & RPLANALYZER_LIBRARY are found
#     RPLANALYZER_LIBRARIES   - Set when RPLANALYZER_LIBRARY is found
#     RPLANALYZER_INCLUDE_DIRS - Set when RPLANALYZER_INCLUDE_DIR is found
#
#     RPLANALYZER_INCLUDE_DIR - where to find header files, etc.
#     RPLANALYZER_LIBRARY     - the rplanalyzer library
#     RPLANALYZER_VERSION_STRING - the version of rplanalyzer found (since CMake 2.8.8)
#

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(RPLANALYZER_INCLUDE_DIR "${SELF_DIR}/../../include/rplanalyzer" ABSOLUTE)
find_library(RPLANALYZER_LIBRARY rplanalyzer)

# handle the QUIETLY and REQUIRED arguments and set RPLANALYZER_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RPLANALYZER REQUIRED_VARS RPLANALYZER_LIBRARY RPLANALYZER_INCLUDE_DIR)

if(RPLANALYZER_FOUND)
  set( RPLANALYZER_LIBRARIES ${RPLANALYZER_LIBRARY} )
  set( RPLANALYZER_INCLUDE_DIRS ${RPLANALYZER_INCLUDE_DIR} )
endif()

mark_as_advanced(RPLANALYZER_INCLUDE_DIR RPLANALYZER_LIBRARY)
