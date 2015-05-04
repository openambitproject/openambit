#  CMakeLists.txt -- build specification for the movescount library
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

project(movescount
  LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../openambit/cmake")

find_package(Qt4 REQUIRED
  COMPONENTS QtCore QtNetwork)
find_package(QJSON 0.8.0 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libambit REQUIRED)

include(${QT_USE_FILE})
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
  ../openambit/
  ../libambit/
  ${QJSON_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIRS}
  )

link_directories(
  ../libambit/
  )

add_library (
  movescount
  SHARED
  movescount.cpp
  movescountjson.cpp
  movescountlogdirentry.cpp
  movescountxml.cpp
  movescountlogchecker.cpp
  )

target_link_libraries(
  movescount
  ${LIBAMBIT_LIBS}
  ${QJSON_LIBRARIES}
  ${ZLIB_LIBRARIES}
  Qt4::QtNetwork
  Qt4::QtCore
  )

set_target_properties(movescount
  PROPERTIES
  VERSION 0.3.0
  SOVERSION 0
  )
install(TARGETS
  movescount
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
install(FILES
  movescount.h
  movescountxml.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/movescount
  )
