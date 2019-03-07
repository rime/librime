set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

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
    ${Opencc_LIBRARY})
set(rime_levers_deps "")

if(MINGW)
  set(rime_core_deps ${rime_core_deps} wsock32 ws2_32)
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
    SOVERSION ${rime_soversion})
  if(XCODE_VERSION)
    set_target_properties(rime PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  install(TARGETS rime DESTINATION ${LIB_INSTALL_DIR})

  if(BUILD_SEPARATE_LIBS)
    add_library(rime-dict ${rime_dict_module_src})
    target_link_libraries(rime-dict
      ${rime_dict_deps}
      ${rime_library})
    set_target_properties(rime-dict PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion})
    if(XCODE_VERSION)
      set_target_properties(rime-dict PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-dict DESTINATION ${LIB_INSTALL_DIR})

    add_library(rime-gears ${rime_gears_src})
    target_link_libraries(rime-gears
      ${rime_gears_deps}
      ${rime_library}
      ${rime_dict_library})
    set_target_properties(rime-gears PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion})
    if(XCODE_VERSION)
      set_target_properties(rime-gears PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-gears DESTINATION ${LIB_INSTALL_DIR})

    add_library(rime-levers ${rime_levers_src})
    target_link_libraries(rime-levers
      ${rime_levers_deps}
      ${rime_library}
      ${rime_dict_library})
    set_target_properties(rime-levers PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion})
    if(XCODE_VERSION)
      set_target_properties(rime-levers PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-levers DESTINATION ${LIB_INSTALL_DIR})

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
        SOVERSION ${rime_soversion})
      if(XCODE_VERSION)
        set_target_properties(rime-plugins PROPERTIES INSTALL_NAME_DIR "@rpath")
      endif()
      install(TARGETS rime-plugins DESTINATION ${LIB_INSTALL_DIR})
    endif()
  endif()
else()
  add_library(rime-static STATIC ${rime_src})
  target_link_libraries(rime-static ${rime_deps})
  set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")
  install(TARGETS rime-static DESTINATION ${LIB_INSTALL_DIR})
endif()
