set(sample_test_output_dir ${PROJECT_BINARY_DIR}/test)

file(GLOB sample_test_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

if(TARGET GTest::gtest)
  set(rime_gtest_target GTest::gtest)
else()
  set(rime_gtest_target ${GTEST_LIBRARIES})
endif()

add_executable(sample_test ${sample_test_src})
target_link_libraries(sample_test
  PRIVATE
  ${sample_library}
  ${rime_library}
  ${rime_gtest_target})
rime_setup_executable(sample_test ${sample_test_output_dir})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(sample_test PRIVATE RIME_IMPORTS)
endif()

add_test(NAME sample_test COMMAND sample_test)
