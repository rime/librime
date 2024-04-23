aux_source_directory(. rime_test_src)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
add_executable(rime_test ${rime_test_src})
target_link_libraries(rime_test
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${GTEST_LIBRARIES})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(rime_test PRIVATE RIME_IMPORTS)
endif(BUILD_SHARED_LIBS)

file(GLOB test_data_files ${PROJECT_SOURCE_DIR}/data/test/*.yaml)
file(COPY ${test_data_files} DESTINATION ${EXECUTABLE_OUTPUT_PATH})

add_test(NAME rime_test
  COMMAND rime_test
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
