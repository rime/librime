if(NOT (WIN32 AND BUILD_SHARED_LIBS))

aux_source_directory(. rime_test_src)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
add_executable(rime_test ${rime_test_src})
target_link_libraries(rime_test
                      ${rime_library} ${rime_gears_library}
                      ${GTEST_LIBRARIES})
add_dependencies(rime_test ${rime_library} ${rime_gears_library})

file(COPY ${PROJECT_SOURCE_DIR}/data/test/config_test.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/test/dictionary_test.dict.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})

set(rime_test_executable ${EXECUTABLE_OUTPUT_PATH}/rime_test${ext})
add_test(rime_test ${rime_test_executable})

endif()
