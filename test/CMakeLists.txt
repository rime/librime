if(WIN32)
  if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS)
    add_custom_target(copy_librime_to_test
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${rime_library}> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${rime_dict_library}> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${rime_gears_library}> ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Copying librime (separated) to test"
    )
  else()
    add_custom_target(copy_librime_to_test
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${rime_library}> ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Copying librime (full) to test"
    )
  endif()
endif()

aux_source_directory(. rime_test_src)
add_executable(rime_test ${rime_test_src})
target_link_libraries(rime_test
  ${rime_library}
  ${rime_dict_library}
  ${rime_gears_library}
  ${rime_levers_library}
  GTest::gtest
  GTest::gtest_main)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(rime_test PRIVATE RIME_IMPORTS)
endif(BUILD_SHARED_LIBS)

add_test(NAME rime_test COMMAND $<TARGET_FILE:rime_test>
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/data/test")
if(WIN32)
  add_dependencies(rime_test copy_librime_to_test)
endif()
