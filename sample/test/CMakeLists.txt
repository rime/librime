set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)

aux_source_directory(. sample_test_src)
add_executable(sample_test ${sample_test_src})
target_link_libraries(sample_test
  ${sample_library}
  ${rime_library}
  ${GTEST_LIBRARIES})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(sample_test PRIVATE RIME_IMPORTS)
endif(BUILD_SHARED_LIBS)

set(sample_test_executable ${EXECUTABLE_OUTPUT_PATH}/sample_test)
add_test(sample_test ${sample_test_executable})
