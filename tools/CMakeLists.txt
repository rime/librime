set(rime_tools_output_dir ${PROJECT_BINARY_DIR}/bin)

set(rime_console_deps
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${rime_levers_library}
  ${rime_plugins_library})

set(rime_api_console_src
  "rime_api_console.cc"
  "completion_catalog.cc"
  "line_editor.cc")
add_executable(rime_api_console ${rime_api_console_src})
target_link_libraries(rime_api_console PRIVATE ${rime_console_deps})
rime_setup_executable(rime_api_console ${rime_tools_output_dir})

set(rime_console_src "rime_console.cc")
add_executable(rime_console ${rime_console_src})
target_compile_definitions(rime_console PRIVATE RIME_IMPORTS)
target_link_libraries(rime_console PRIVATE ${rime_console_deps})
rime_setup_executable(rime_console ${rime_tools_output_dir})

set(rime_deployer_src "rime_deployer.cc")
add_executable(rime_deployer ${rime_deployer_src})
target_compile_definitions(rime_deployer PRIVATE RIME_IMPORTS)
target_link_libraries(rime_deployer
  PRIVATE ${rime_library}
  ${rime_dict_library}
  ${rime_levers_library})
rime_setup_executable(rime_deployer ${rime_tools_output_dir})

set(rime_dict_manager_src "rime_dict_manager.cc")
add_executable(rime_dict_manager ${rime_dict_manager_src})
target_link_libraries(rime_dict_manager
  PRIVATE ${rime_library}
  ${rime_dict_library}
  ${rime_levers_library})
rime_setup_executable(rime_dict_manager ${rime_tools_output_dir})

set(rime_patch_src "rime_patch.cc")
add_executable(rime_patch ${rime_patch_src})
target_link_libraries(rime_patch
  PRIVATE ${rime_library}
  ${rime_levers_library})
rime_setup_executable(rime_patch ${rime_tools_output_dir})

install(TARGETS rime_deployer RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS rime_dict_manager RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS rime_patch RUNTIME DESTINATION ${BIN_INSTALL_DIR})

# do not work with Windows DLL; interfaces to dict are missing DLL export.
if(NOT WIN32 OR NOT BUILD_SHARED_LIBS)
  set(rime_table_decompiler_src "rime_table_decompiler.cc")
  add_executable(rime_table_decompiler ${rime_table_decompiler_src})
  target_compile_definitions(rime_deployer PRIVATE RIME_IMPORTS)
  target_link_libraries(rime_table_decompiler
    PRIVATE ${rime_library}
    ${rime_dict_library})
  rime_setup_executable(rime_table_decompiler ${rime_tools_output_dir})

  install(TARGETS rime_table_decompiler RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif()

set(rime_tools_data_files)
foreach(data_file
    default.yaml
    symbols.yaml
    essay.txt
    luna_pinyin.dict.yaml
    luna_pinyin.schema.yaml
    cangjie5.dict.yaml
    cangjie5.schema.yaml)
  list(APPEND rime_tools_data_files "${PROJECT_SOURCE_DIR}/data/minimal/${data_file}")
endforeach()
add_custom_target(rime_tools_data ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${rime_tools_output_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${rime_tools_data_files} "${rime_tools_output_dir}")
foreach(tool_target
    rime_api_console
    rime_console
    rime_deployer
    rime_dict_manager
    rime_patch
    rime_table_decompiler)
  if(TARGET ${tool_target})
    add_dependencies(${tool_target} rime_tools_data)
  endif()
endforeach()
