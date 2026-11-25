# No project() here: it would reset PROJECT_SOURCE_DIR for nested directories.
set(SAMPLE_VERSION 1.0.0)
set(SAMPLE_SOVERSION 1)

file(GLOB sample_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

set(sample_library rime-sample)
set(sample_deps ${rime_library})
set(sample_modules "sample")

if(BUILD_SAMPLE)
  # to build the sample plugin, console app and tests, set BUILD_SAMPLE=ON
  message(STATUS "${sample_library} provides modules: ${sample_modules}")
  add_library(${sample_library} ${sample_src})
  target_link_libraries(${sample_library} PUBLIC ${sample_deps} PRIVATE rime-build-options)
  set_target_properties(${sample_library}
    PROPERTIES
    VERSION ${SAMPLE_VERSION}
    SOVERSION ${SAMPLE_SOVERSION}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  if(XCODE_VERSION)
    set_target_properties(${sample_library}
      PROPERTIES
      INSTALL_NAME_DIR "@rpath")
  endif(XCODE_VERSION)
  install(TARGETS ${sample_library}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_subdirectory(tools)
  if(BUILD_TEST)
    add_subdirectory(test)
  endif()
else()
  # to build as a standard Rime plugin, link or copy the directory to plugins/
  add_library(rime-sample-objs OBJECT ${sample_src})
  target_link_libraries(rime-sample-objs PRIVATE rime-build-options)
  if(BUILD_SHARED_LIBS)
    set_target_properties(rime-sample-objs
      PROPERTIES
      POSITION_INDEPENDENT_CODE ON)
  endif()

  set(plugin_name ${sample_library} PARENT_SCOPE)
  set(plugin_objs $<TARGET_OBJECTS:rime-sample-objs> PARENT_SCOPE)
  set(plugin_deps ${sample_deps} PARENT_SCOPE)
  set(plugin_modules ${sample_modules} PARENT_SCOPE)
endif()
