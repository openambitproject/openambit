if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  execute_process(
        COMMAND brew --prefix qt@5
        RESULT_VARIABLE BREW_QT5_RES
        OUTPUT_VARIABLE BREW_QT5_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
        COMMAND brew --prefix hidapi
        RESULT_VARIABLE BREW_HIDAPI_RES
        OUTPUT_VARIABLE BREW_HIDAPI_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
  )
 set(Qt5Core_DIR ${BREW_QT5_PREFIX}/lib/cmake/Qt5Core)
 set(Qt5Widgets_DIR ${BREW_QT5_PREFIX}/lib/cmake/Qt5Widgets)
 set(Qt5LinguistTools_DIR ${BREW_QT5_PREFIX}/lib/cmake/Qt5LinguistTools)
 set(Qt5Network_DIR ${BREW_QT5_PREFIX}/lib/cmake/Qt5Network)
 set(Qt5NetworkConfig_DIR ${BREW_QT5_PREFIX}/lib/cmake/Qt5NetworkConfig)
 set(CMAKE_SHARED_MODULE_SUFFIX ".dylib")
 set(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
endif()
