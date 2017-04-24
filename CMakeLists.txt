#  CMakeLists.txt -- Openambit top-level build specification
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

option(USE_QT5 "Build against Qt5" OFF)
option(BUILD_EXTRAS "Build examples and wireshark dissector" OFF)

add_subdirectory(src/libambit)
add_subdirectory(src/movescount)
add_subdirectory(src/openambit)

if (NOT ${LIBAMBIT_FOUND})
  add_dependencies(movescount ambit)
  add_dependencies(openambit ambit)
endif ()
if (NOT ${MOVESCOUNT_FOUND})
  add_dependencies(openambit movescount)
endif ()

if (BUILD_EXTRAS)
  add_subdirectory(src/example)
  add_subdirectory(wireshark_dissector)

  if (NOT ${LIBAMBIT_FOUND})
    add_dependencies(ambitconsole ambit)
  endif ()

endif ()
