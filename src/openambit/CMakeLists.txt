cmake_minimum_required(VERSION 2.8.5)
project (OPENAMBIT)

set (OPENAMBIT_VERSION 0.3)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${OPENAMBIT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(Qt4 REQUIRED)
find_package(libambit REQUIRED)
find_package(UDev REQUIRED)
find_package(ZLIB REQUIRED)
find_package(QJSON 0.8.0 REQUIRED)

include(${QT_USE_FILE})
include(GNUInstallDirs)

include_directories (
  ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${QT_QTNETWORK_INCLUDE_DIR}
  ${LIBAMBIT_INCLUDE_DIR}
)

link_directories(
  ${LIBAMBIT_LIBS_DIR}
)

set ( openambit_HDRS
  logentry.h
)

set ( openambit_SRCS
  main.cpp
  mainwindow.cpp
  devicemanager.cpp
  settingsdialog.cpp
  settings.cpp
  logstore.cpp
  logentry.cpp
  udevlistener.cpp
  confirmbetadialog.cpp
  single_application.cpp
)

set ( openambit_UIS
  mainwindow.ui
  settingsdialog.ui
  confirmbetadialog.ui
)

set ( openambit_RSCS
  resources.qrc
)

set ( openambit_MOCS
  mainwindow.h
  devicemanager.h
  settingsdialog.h
  settings.h
  logstore.h
  udevlistener.h
  confirmbetadialog.h
  single_application.h
)

set ( APP_ICON ${PROJECT_SOURCE_DIR}/icons/icon_disconnected.png )

set ( CMAKE_INSTALL_UDEVRULESDIR /lib/udev/rules.d
      CACHE PATH "Where to install udev rules"
)
mark_as_advanced ( CMAKE_INSTALL_UDEVRULESDIR )

add_subdirectory("${PROJECT_SOURCE_DIR}/movescount")

QT4_WRAP_UI(UIS ${openambit_UIS})
QT4_ADD_RESOURCES(RSCS ${openambit_RSCS})
QT4_WRAP_CPP(MOCS ${openambit_MOCS})

add_definitions( -DAPP_VERSION="${OPENAMBIT_VERSION}" )

add_executable ( openambit ${openambit_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )

target_link_libraries ( openambit  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${LIBAMBIT_LIBS} ${UDEV_LIBS} ${ZLIB_LIBRARY} ${QJSON_LIBRARIES} )

install ( TARGETS openambit DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES ${OPENAMBIT_SOURCE_DIR}/deployment/99-suunto-ambit.rules
          DESTINATION ${CMAKE_INSTALL_UDEVRULESDIR}
          COMPONENT system
)
install ( FILES ${OPENAMBIT_SOURCE_DIR}/deployment/openambit.desktop
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications )
install ( FILES ${APP_ICON}
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
          RENAME openambit.png )
