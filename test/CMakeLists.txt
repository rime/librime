aux_source_directory(. RIME_TEST_SRC)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
add_executable(rime_test ${RIME_TEST_SRC})
target_link_libraries(rime_test rime ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GTEST_LIBRARIES} ${YamlCpp_LIBRARY})
add_dependencies(rime_test rime)

file(COPY ${PROJECT_SOURCE_DIR}/data/test.yaml DESTINATION ${EXECUTABLE_OUTPUT_PATH})
set(RIME_TEST_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/rime_test${EXT})
add_custom_command(TARGET rime_test
                   POST_BUILD
                   COMMAND ${RIME_TEST_EXECUTABLE}
                   WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
