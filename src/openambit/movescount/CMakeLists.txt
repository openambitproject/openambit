set ( OpenAmbitQt_SRCS
  ${OpenAmbitQt_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/movescount.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountjson.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogchecker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogdirentry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountxml.cpp
  PARENT_SCOPE
)

set ( OpenAmbitQt_MOCS
  ${OpenAmbitQt_MOCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/movescount.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountjson.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogchecker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountlogdirentry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movescountxml.h
  PARENT_SCOPE
)

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${openambit_SRCS} ${openambit_MOCS} PARENT_SCOPE)
