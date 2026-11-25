file(GLOB rime_test_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

set(rime_test_output_dir ${PROJECT_BINARY_DIR}/test)

if(TARGET GTest::gtest)
  set(rime_gtest_target GTest::gtest)
else()
  set(rime_gtest_target ${GTEST_LIBRARIES})
endif()

add_executable(rime_test ${rime_test_src})
target_link_libraries(rime_test
  PRIVATE ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${rime_levers_library}
  ${rime_gtest_target})
rime_setup_executable(rime_test ${rime_test_output_dir})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(rime_test PRIVATE RIME_IMPORTS)
endif()
if(NOT Opencc_DICT_DIR)
  message(FATAL_ERROR "Could not locate the installed opencc dict directory "
                       "(e.g. t2s.json); required to build rime_test.")
endif()
target_compile_definitions(rime_test PRIVATE
  RIME_OPENCC_DICT_DIR="${Opencc_DICT_DIR}")

file(GLOB test_data_files ${PROJECT_SOURCE_DIR}/data/test/*.yaml)
add_custom_target(rime_test_data ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${rime_test_output_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${test_data_files} "${rime_test_output_dir}")
add_dependencies(rime_test rime_test_data)

add_test(NAME rime_test
  COMMAND rime_test
  WORKING_DIRECTORY ${rime_test_output_dir})
