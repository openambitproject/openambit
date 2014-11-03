# - Find libambit
# If found, this will define
#
# LIBAMBIT_FOUND - system has libambit
# LIBAMBIT_INCLUDE_DIR - the libambit include directory
# LIBAMBIT_LIBS - the libambit libraries

find_path(LIBAMBIT_INCLUDE_DIR NAMES libambit.h
  PATHS
  ../libambit
  ${CMAKE_SOURCE_DIR}/src/libambit
)

find_library(LIBAMBIT_LIBS_PATH NAMES libambit.so
  PATHS
  ../../libambit-build
)

find_library(LIBAMBIT_LIBS NAMES ambit
  PATHS
  ../../libambit-build
)

if(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)
  set(LIBAMBIT_FOUND TRUE CACHE INTERNAL "libambit found")
  message(STATUS "Found libambit: ${LIBAMBIT_INCLUDE_DIR}, ${LIBAMBIT_LIBS}")
else(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)
  set(LIBAMBIT_FOUND FALSE CACHE INTERNAL "libambit found")
  message(STATUS "libambit not found.")

  set(LIBAMBIT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/libambit/)
  set(LIBAMBIT_LIBS ${CMAKE_BINARY_DIR}/src/libambit/libambit.so.0)
  set(LIBAMBIT_FOUND true)
endif(LIBAMBIT_INCLUDE_DIR AND LIBAMBIT_LIBS)

mark_as_advanced(LIBAMBIT_INCLUDE_DIR LIBAMBIT_LIBS)
