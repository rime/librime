file(GLOB rime_test_src ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
option(BUILD_USER_DICT_BENCHMARK
  "Build the user dictionary performance benchmark (for cache on/off comparison)"
  OFF)
if(NOT BUILD_USER_DICT_BENCHMARK)
  list(REMOVE_ITEM rime_test_src
    ${CMAKE_CURRENT_SOURCE_DIR}/user_dictionary_benchmark.cc)
endif()
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
if(NOT Opencc_DICT_DIR)
  message(FATAL_ERROR "Could not locate the installed opencc dict directory "
                       "(e.g. t2s.json); required to build rime_test.")
endif()
target_compile_definitions(rime_test PRIVATE
  RIME_OPENCC_DICT_DIR="${Opencc_DICT_DIR}")

file(GLOB test_data_files ${PROJECT_SOURCE_DIR}/data/test/*.yaml)
file(COPY ${test_data_files} DESTINATION ${EXECUTABLE_OUTPUT_PATH})

add_test(NAME rime_test
  COMMAND rime_test
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
