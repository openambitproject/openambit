cmake_minimum_required (VERSION 2.6)
project (OPENAMBIT)

# Where to lookup modules
set (CMAKE_MODULE_PATH "${OPENAMBIT_SOURCE_DIR}/cmake")

add_subdirectory (libambit)
add_subdirectory (example)
