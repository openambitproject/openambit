cmake_minimum_required(VERSION 2.8.5)
project (OPENAMBIT CXX)

set (OPENAMBIT_VERSION HEAD)

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
  deviceinfo.h
  logentry.h
)

set ( openambit_SRCS
  confirmbetadialog.cpp
  deviceinfo.cpp
  devicemanager.cpp
  logentry.cpp
  logstore.cpp
  logview.cpp
  main.cpp
  mainwindow.cpp
  settings.cpp
  settingsdialog.cpp
  signalhandler.cpp
  single_application.cpp
  udevlistener.cpp
)

set ( openambit_UIS
  confirmbetadialog.ui
  mainwindow.ui
  settingsdialog.ui
)

set ( openambit_RSCS
  resources.qrc
)

set ( openambit_MOCS
  confirmbetadialog.h
  devicemanager.h
  logstore.h
  logview.h
  mainwindow.h
  settings.h
  settingsdialog.h
  signalhandler.h
  single_application.h
  udevlistener.h
)

set (FILES_TO_TRANSLATE ${openambit_SRCS} ${openambit_UIS} ${openambit_HDRS} ${openambit_MOCS})

set ( APP_ICON ${PROJECT_SOURCE_DIR}/icons/icon_disconnected.png )

add_subdirectory("${PROJECT_SOURCE_DIR}/movescount")

######### Translations
file (GLOB TRANSLATIONS_FILES translations/*.ts)

option (UPDATE_TRANSLATIONS "Update source translation translations/*.ts")
if (UPDATE_TRANSLATIONS)
  qt4_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})
else (UPDATE_TRANSLATIONS)
  qt4_add_translation(QM_FILES ${TRANSLATIONS_FILES})
endif (UPDATE_TRANSLATIONS)

# Create translations QRC file - ts.qrc
set(TRANSLATIONS_QRC "${CMAKE_CURRENT_BINARY_DIR}/ts.qrc")
file(WRITE ${TRANSLATIONS_QRC} "<RCC>\n\t<qresource prefix=\"/translations\">")
foreach(QM_FILE ${QM_FILES})
    get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
    file(APPEND ${TRANSLATIONS_QRC} "\n\t\t<file alias=\"${QM_FILE_NAME}\">${QM_FILE_NAME}</file>")
endforeach()
file(APPEND ${TRANSLATIONS_QRC} "\n\t</qresource>\n</RCC>")
list(APPEND openambit_RSCS ${TRANSLATIONS_QRC})

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

######### Translations

QT4_WRAP_UI(UIS ${openambit_UIS})
QT4_ADD_RESOURCES(RSCS ${openambit_RSCS})
QT4_WRAP_CPP(MOCS ${openambit_MOCS})

add_definitions( -DAPP_VERSION="${OPENAMBIT_VERSION}" )

add_executable ( openambit ${openambit_SRCS} ${UIS} ${RSCS} ${MOCS} )

target_link_libraries ( openambit  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${LIBAMBIT_LIBS} ${UDEV_LIBS} ${ZLIB_LIBRARY} ${QJSON_LIBRARIES} )

install ( TARGETS openambit DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES ${OPENAMBIT_SOURCE_DIR}/deployment/openambit.desktop
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications )
install ( FILES ${APP_ICON}
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
          RENAME openambit.png )

