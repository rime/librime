set(sample_tools_output_dir ${PROJECT_BINARY_DIR}/bin)

set(sample_console_src "sample_console.cc")
add_executable(sample_console ${sample_console_src})
target_link_libraries(sample_console
  PRIVATE
  ${sample_library}
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${rime_levers_library})
rime_setup_executable(sample_console ${sample_tools_output_dir})

set(sample_tools_data_files
  "${CMAKE_CURRENT_SOURCE_DIR}/default.yaml"
  "${CMAKE_CURRENT_SOURCE_DIR}/sample.schema.yaml")
add_custom_target(sample_tools_data ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${sample_tools_output_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${sample_tools_data_files} "${sample_tools_output_dir}")
add_dependencies(sample_console sample_tools_data)
