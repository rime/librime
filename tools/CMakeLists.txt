set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if(BUILD_SHARED_LIBS)
  set(RIME_LIBRARY rime)
else(BUILD_SHARED_LIBS)
  set(RIME_LIBRARY rime-static)
endif(BUILD_SHARED_LIBS)

set(RIME_API_CONSOLE_SRC "rime_api_console.cc")
add_executable(rime_api_console ${RIME_API_CONSOLE_SRC})
target_link_libraries(rime_api_console ${RIME_LIBRARY})
add_dependencies(rime_api_console ${RIME_LIBRARY})

set(RIME_CONSOLE_SRC "rime_console.cc")
add_executable(rime_console ${RIME_CONSOLE_SRC})
target_link_libraries(rime_console ${RIME_LIBRARY})
add_dependencies(rime_console ${RIME_LIBRARY})

set(RIME_DICT_MANAGER_SRC "rime_dict_manager.cc")
add_executable(rime_dict_manager ${RIME_DICT_MANAGER_SRC})
target_link_libraries(rime_dict_manager ${RIME_LIBRARY})
add_dependencies(rime_dict_manager ${RIME_LIBRARY})

set(RIME_DEPLOYER_SRC "rime_deployer.cc")
add_executable(rime_deployer ${RIME_DEPLOYER_SRC})
target_link_libraries(rime_deployer ${RIME_LIBRARY})
add_dependencies(rime_deployer ${RIME_LIBRARY})

install(TARGETS rime_dict_manager DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS rime_deployer DESTINATION ${BIN_INSTALL_DIR})

file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/default.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/symbols.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/essay.kct
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/luna_pinyin.dict.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/luna_pinyin.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/cangjie5.dict.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/minimal/cangjie5.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
