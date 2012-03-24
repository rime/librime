set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(RIME_CONSOLE_SRC "rime_console.cc")
add_executable(rime_console ${RIME_CONSOLE_SRC})
target_link_libraries(rime_console rime-static)
add_dependencies(rime_console rime-static)

set(RIME_API_CONSOLE_SRC "rime_api_console.cc")
add_executable(rime_api_console ${RIME_API_CONSOLE_SRC})
target_link_libraries(rime_api_console rime-static)
add_dependencies(rime_api_console rime-static)

set(RIME_DICT_MANAGER_SRC "rime_dict_manager.cc")
add_executable(rime_dict_manager ${RIME_DICT_MANAGER_SRC})
target_link_libraries(rime_dict_manager rime-static)
add_dependencies(rime_dict_manager rime-static)

file(COPY ${PROJECT_SOURCE_DIR}/data/default.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/essay.kct
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/schema/luna_pinyin.dict.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/schema/luna_pinyin.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/schema/cangjie5.dict.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/schema/cangjie5.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
