# - Find Movescount
# If found, this will define
#
# MOVESCOUNT_FOUND - system has the movescount library
# MOVESCOUNT_INCLUDE_DIR - the movescount include directory
# MOVESCOUNT_LIBS - the movescount libraries

find_path(MOVESCOUNT_INCLUDE_DIR NAMES movescount.h
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../movescount NO_DEFAULT_PATH
)
find_path(MOVESCOUNT_INCLUDE_DIR NAMES movescount.h)

find_library(MOVESCOUNT_LIBS NAMES movescount
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/../movescount-build NO_DEFAULT_PATH
)
find_library(MOVESCOUNT_LIBS NAMES movescount)

if(MOVESCOUNT_INCLUDE_DIR AND MOVESCOUNT_LIBS)
  set(MOVESCOUNT_FOUND TRUE CACHE INTERNAL "Movescount found")
  message(STATUS "Found Movescount: ${MOVESCOUNT_INCLUDE_DIR}, ${MOVESCOUNT_LIBS}")
else(MOVESCOUNT_INCLUDE_DIR AND LIBMOVESCOUNT_LIBS)
  set(MOVESCOUNT_FOUND FALSE CACHE INTERNAL "Movescount found")
  set(MOVESCOUNT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
  set(MOVESCOUNT_LIBS ${CMAKE_CURRENT_BINARY_DIR}/../movescount/libmovescount.so.0)
  message(STATUS
    "Movescount not found, building from source in ${MOVESCOUNT_INCLUDE_DIR}")
endif(MOVESCOUNT_INCLUDE_DIR AND MOVESCOUNT_LIBS)

mark_as_advanced(MOVESCOUNT_INCLUDE_DIR MOVESCOUNT_LIBS)
