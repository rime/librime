aux_source_directory(. SAMPLE_TEST_SRC)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
add_executable(sample_test ${SAMPLE_TEST_SRC})
target_link_libraries(sample_test rime-sample ${Rime_LIBRARIES} ${GTEST_LIBRARIES})
add_dependencies(sample_test rime-sample)

set(SAMPLE_TEST_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/sample_test)
add_test(sample_test ${SAMPLE_TEST_EXECUTABLE})
