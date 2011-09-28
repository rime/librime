aux_source_directory(. RIME_TEST_SRC)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
add_executable(rime_test ${RIME_TEST_SRC})
target_link_libraries(rime_test
                      rime-static
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${GTEST_LIBRARIES})
add_dependencies(rime_test rime-static)

file(COPY ${PROJECT_SOURCE_DIR}/data/test.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/dictionary_test.dict.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})

if(NOT MSVC AND NOT XCODE_VERSION)
set(RIME_TEST_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/rime_test${EXT})
add_custom_command(TARGET rime_test
                   POST_BUILD
                   COMMAND ${RIME_TEST_EXECUTABLE}
                   WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
endif(NOT MSVC AND NOT XCODE_VERSION)
