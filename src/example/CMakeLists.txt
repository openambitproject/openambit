include_directories(
  ${OPENAMBIT_SOURCE_DIR}/libambit
)

link_directories(
  ${OPENAMBIT_BINARY_DIR}/libambit
)

add_executable(
  ambitconsole ambitconsole.c
)

target_link_libraries(
  ambitconsole ambit
)
