set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(sample_console_src "sample_console.cc")
add_executable(sample_console ${sample_console_src})
target_link_libraries(sample_console
  ${sample_library}
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${rime_levers_library})

file(COPY ${PROJECT_SOURCE_DIR}/tools/default.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/tools/sample.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
