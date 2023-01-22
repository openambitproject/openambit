#  Copyright (C) 2015 Olaf Meeuwissen
#
#  This file is part of Openambit.
#
#  Openambit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published
#  by the Free Software Foundation, either version 3 of the License,
#  or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 3.0)

project(Openambit
  LANGUAGES NONE)

option(BUILD_EXTRAS "Build examples" ON)
option(BUILD_WS "Build wireshark plugin" OFF)

# TODO: Enable this switch (for mac)
# option(BUILD_MOVESCOUNT "Build movescount lib" ON)
option(BUILD_UNIT_TESTS "Build unittests" ON)

include(${CMAKE_SOURCE_DIR}/Mac.cmake)

add_subdirectory(src/libambit)
if (BUILD_MOVESCOUNT)
  add_subdirectory(src/movescount)
endif()
add_subdirectory(src/openambit)
add_subdirectory(src/openambit-cli)
add_subdirectory(src/openambit-routes)
if(BUILD_UNIT_TESTS)
  add_subdirectory(src/unittest)
endif()

if (NOT ${LIBAMBIT_FOUND})
  if(MOVESCOUNT)
    add_dependencies(movescount ambit)
  endif ()
  add_dependencies(openambit ambit)
  add_dependencies(openambit-cli ambit)
  add_dependencies(openambit-routes ambit)
  if(BUILD_UNIT_TESTS)
    add_dependencies(unittest ambit)
  endif()
endif ()
if(MOVESCOUNT)
if (NOT ${MOVESCOUNT_FOUND})
  add_dependencies(openambit movescount)
  add_dependencies(openambit-cli movescount)
  if(BUILD_UNIT_TESTS)
    add_dependencies(unittest movescount)
  endif()
endif ()
endif ()

if (BUILD_EXTRAS)
  add_subdirectory(src/example)
  if (NOT ${LIBAMBIT_FOUND})
    add_dependencies(ambitconsole ambit)
  endif ()
endif ()
if (BUILD_WS)
  add_subdirectory(wireshark_dissector)
endif()

