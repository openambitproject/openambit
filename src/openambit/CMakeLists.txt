cmake_minimum_required(VERSION 2.8.5)
project (OPENAMBIT CXX)

set (OPENAMBIT_VERSION 0.4)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${OPENAMBIT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(libambit REQUIRED)
find_package(Movescount REQUIRED)
IF (!WIN32)
find_package(UDev REQUIRED)
ELSE (!WIN32)
set(UDEV_LIBS "")
ENDIF (!WIN32)
find_package(Qt5Network REQUIRED)

include(GNUInstallDirs)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBAMBIT_INCLUDE_DIR}
  ..
)

link_directories(
  ${LIBAMBIT_LIBS_DIR}
  ${MOVESCOUNT_LIBS_DIR}
)

set(openambit_HDRS
  confirmbetadialog.h
  devicemanager.h
  logview.h
  mainwindow.h
  settings.h
  settingsdialog.h
  signalhandler.h
  single_application.h
  udevlistener.h
)

set(openambit_SRCS
  confirmbetadialog.cpp
  devicemanager.cpp
  logview.cpp
  main.cpp
  mainwindow.cpp
  settings.cpp
  settingsdialog.cpp
  signalhandler.cpp
  single_application.cpp
  udevlistener.cpp
)

set(openambit_UIS
  confirmbetadialog.ui
  mainwindow.ui
  settingsdialog.ui
)

set(openambit_RSCS
  resources.qrc
)

set(FILES_TO_TRANSLATE ${openambit_SRCS} ${openambit_UIS} ${openambit_MOCS})

set(APP_ICON ${PROJECT_SOURCE_DIR}/icons/icon_disconnected.png)

######### Translations
file(GLOB TRANSLATIONS_FILES translations/*.ts)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts")
if(UPDATE_TRANSLATIONS)
  qt5_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})
else(UPDATE_TRANSLATIONS)
  qt5_add_translation(QM_FILES ${TRANSLATIONS_FILES})
endif(UPDATE_TRANSLATIONS)

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

QT5_WRAP_UI(UIS ${openambit_UIS})
QT5_ADD_RESOURCES(RSCS ${openambit_RSCS})

set(CMAKE_AUTOMOC ON)

add_definitions(-DAPP_VERSION="${OPENAMBIT_VERSION}")

add_executable(openambit ${openambit_HDRS} ${openambit_SRCS} ${UIS} ${RSCS})

target_link_libraries(openambit ${LIBAMBIT_LIBS} ${MOVESCOUNT_LIBS} ${UDEV_LIBS} Qt5::Core Qt5::Widgets Qt5::Network )

install(TARGETS openambit DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${OPENAMBIT_SOURCE_DIR}/deployment/openambit.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES ${OPENAMBIT_SOURCE_DIR}/deployment/openambit.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/appdata )
install(FILES ${APP_ICON}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
        RENAME openambit.png)
