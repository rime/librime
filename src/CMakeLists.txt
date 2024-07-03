aux_source_directory(. rime_api_src)
aux_source_directory(rime rime_base_src)
aux_source_directory(rime/algo rime_algo_src)
aux_source_directory(rime/config rime_config_src)
aux_source_directory(rime/dict rime_dict_src)
aux_source_directory(rime/gear rime_gears_src)
aux_source_directory(rime/lever rime_levers_src)
if(rime_plugins_library)
  aux_source_directory(../plugins rime_plugins_src)
endif()

set(rime_core_module_src
  ${rime_api_src}
  ${rime_base_src}
  ${rime_config_src})

# add rc info for windows MSVC build
if(WIN32)
  include(${CMAKE_SOURCE_DIR}/cmake/AddRCInfo.cmake)
endif()

set(rime_dict_module_src
  ${rime_algo_src}
  ${rime_dict_src})

if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS)
  set(rime_src ${rime_core_module_src})
else()
  set(rime_src
      ${rime_core_module_src}
      ${rime_dict_module_src}
      ${rime_gears_src}
      ${rime_levers_src}
      ${rime_plugins_src}
      ${rime_plugins_objs})
endif()

set(rime_optional_deps "")
if(Gflags_FOUND)
  set(rime_optional_deps ${rime_optional_deps} ${Gflags_LIBRARY})
endif()
if(ENABLE_EXTERNAL_PLUGINS)
  set(rime_optional_deps ${rime_optional_deps} dl)
endif()

set(rime_core_deps
    ${Boost_LIBRARIES}
    ${Glog_LIBRARY}
    ${YamlCpp_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${rime_optional_deps})
set(rime_dict_deps
    ${LevelDb_LIBRARY}
    ${Marisa_LIBRARY})
set(rime_gears_deps
    ${ICONV_LIBRARIES}
    ${ICU_LIBRARIES}
    ${Opencc_LIBRARY})
set(rime_levers_deps "")

if(Glog_FOUND AND WIN32)
  # TODO: in glog v0.7.0, someone at Google forget to add dbghelp.lib to the linker library list.
  set(rime_core_deps ${rime_core_deps} dbghelp)
endif()

if(MINGW)
  # fix: bcrypt for boost uuid issue
  # https://github.com/boostorg/uuid/issues/68
  set(rime_core_deps ${rime_core_deps} wsock32 ws2_32 bcrypt)
endif()

if(BUILD_SEPARATE_LIBS)
  set(rime_deps ${rime_core_deps})
else()
  set(rime_deps
    ${rime_core_deps}
    ${rime_dict_deps}
    ${rime_gears_deps}
    ${rime_levers_deps}
    ${rime_plugins_deps})
endif()

if(BUILD_SHARED_LIBS)
  add_library(rime ${rime_src})
  target_link_libraries(rime ${rime_deps})
  set_target_properties(rime PROPERTIES
    DEFINE_SYMBOL "RIME_EXPORTS"
    VERSION ${rime_version}
    SOVERSION ${rime_soversion}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  if(XCODE_VERSION)
    set_target_properties(rime PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  install(TARGETS rime DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:rime>
      DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
      OPTIONAL)
  endif()
  if(BUILD_SEPARATE_LIBS)
    add_library(rime-dict ${rime_dict_module_src})
    target_link_libraries(rime-dict
      ${rime_dict_deps}
      ${rime_library})
    set_target_properties(rime-dict PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion}
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    if(XCODE_VERSION)
      set_target_properties(rime-dict PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-dict DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

    add_library(rime-gears ${rime_gears_src})
    target_link_libraries(rime-gears
      ${rime_gears_deps}
      ${rime_library}
      ${rime_dict_library})
    set_target_properties(rime-gears PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion}
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    if(XCODE_VERSION)
      set_target_properties(rime-gears PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-gears DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

    add_library(rime-levers ${rime_levers_src})
    target_link_libraries(rime-levers
      ${rime_levers_deps}
      ${rime_library}
      ${rime_dict_library})
    set_target_properties(rime-levers PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion}
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    if(XCODE_VERSION)
      set_target_properties(rime-levers PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-levers DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

    if(rime_plugins_library)
      add_library(rime-plugins
        ${rime_plugins_src}
        ${rime_plugins_objs})
      target_link_libraries(rime-plugins
        ${rime_plugins_deps}
        ${rime_library}
        ${rime_dict_library}
        ${rime_gears_library})
      set_target_properties(rime-plugins PROPERTIES
        VERSION ${rime_version}
        SOVERSION ${rime_soversion}
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
      if(XCODE_VERSION)
        set_target_properties(rime-plugins PROPERTIES INSTALL_NAME_DIR "@rpath")
      endif()
      install(TARGETS rime-plugins DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
    endif()
  endif()
else()
  add_library(rime-static STATIC ${rime_src})
  target_link_libraries(rime-static ${rime_deps})
  set_target_properties(rime-static PROPERTIES
    OUTPUT_NAME "rime" PREFIX "lib"
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  install(TARGETS rime-static DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
endif()
