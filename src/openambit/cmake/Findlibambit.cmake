# - Find libambit
# If found, this will define
#
# LIBAMBIT_FOUND - system has libambit
# LIBAMBIT_INCLUDE_DIR - the libambit include directory
# LIBAMBIT_LIBS - the libambit libraries

find_path(LIBAMBIT_INCLUDE_DIR NAMES libambit.h
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../libambit NO_DEFAULT_PATH
)
find_path(LIBAMBIT_INCLUDE_DIR NAMES libambit.h)

find_library(LIBAMBIT_LIBS NAMES ambit
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/../libambit-build NO_DEFAULT_PATH
)
find_library(LIBAMBIT_LIBS NAMES ambit)

if(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)
  set(LIBAMBIT_FOUND TRUE CACHE INTERNAL "libambit found")
  message(STATUS "Found libambit: ${LIBAMBIT_INCLUDE_DIR}, ${LIBAMBIT_LIBS}")
else(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)
  set(LIBAMBIT_FOUND FALSE CACHE INTERNAL "libambit found")
  set(LIBAMBIT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libambit/)
  set(LIBAMBIT_LIBS ${CMAKE_CURRENT_BINARY_DIR}/../libambit/libambit.so.0)
  message(STATUS
    "libambit not found, building from source in ${LIBAMBIT_INCLUDE_DIR}")
endif(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)

mark_as_advanced(LIBAMBIT_INCLUDE_DIR LIBAMBIT_LIBS)
