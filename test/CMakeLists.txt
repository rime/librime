aux_source_directory(. rime_test_src)
add_executable(rime_test ${rime_test_src})
target_link_libraries(rime_test
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  GTest::gtest
  GTest::gtest_main)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(rime_test PRIVATE RIME_IMPORTS)
endif(BUILD_SHARED_LIBS)

add_test(NAME rime_test COMMAND rime_test
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/data/test")
