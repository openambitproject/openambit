set ( openambit_SRCS
  ${openambit_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/movescount.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountjson.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogdirentry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountxml.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogchecker.cpp
  PARENT_SCOPE
)

set ( openambit_MOCS
  ${openambit_MOCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/movescount.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountjson.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogdirentry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountxml.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogchecker.h
  PARENT_SCOPE
)
