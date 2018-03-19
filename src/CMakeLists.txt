set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. rime_src_api)
aux_source_directory(rime rime_src_base)
aux_source_directory(rime/algo rime_src_algo)
aux_source_directory(rime/config rime_src_config)
aux_source_directory(rime/dict rime_src_dict)
aux_source_directory(rime/gear rime_src_gear)
aux_source_directory(rime/lever rime_src_lever)
aux_source_directory(../legacy/src rime_src_legacy)

if(BUILD_SEPARATE_LIBS)
  set(rime_src
      ${rime_src_api}
      ${rime_src_base}
      ${rime_src_config})
  set(librime_gears_src
      ${rime_src_algo}
      ${rime_src_dict}
      ${rime_src_gear}
      ${rime_src_lever}
      ${rime_src_legacy})
else()
  set(rime_src
      ${rime_src_api}
      ${rime_src_base}
      ${rime_src_algo}
      ${rime_src_config}
      ${rime_src_dict}
      ${rime_src_gear}
      ${rime_src_lever}
      ${rime_src_legacy})
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
set(rime_extra_deps
    ${ICONV_LIBRARIES}
    ${KyotoCabinet_LIBRARY}
    ${LevelDb_LIBRARY}
    ${Marisa_LIBRARY}
    ${Opencc_LIBRARY}
    ${ZLIB_LIBRARY})

if(MINGW)
  set(rime_core_deps ${rime_core_deps} wsock32 ws2_32)
endif()

if(BUILD_SEPARATE_LIBS)
  set(rime_deps ${rime_core_deps})
  set(rime_gears_deps ${rime_library} ${rime_extra_deps})
else()
  set(rime_deps ${rime_core_deps} ${rime_extra_deps})
endif()

if(BUILD_SHARED_LIBS)
  add_library(rime ${rime_src})
  target_link_libraries(rime ${rime_deps})
  set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
  set_target_properties(rime PROPERTIES VERSION ${rime_version} SOVERSION ${rime_soversion})
  if(XCODE_VERSION)
    set_target_properties(rime PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  install(TARGETS rime DESTINATION ${LIB_INSTALL_DIR})
  if(BUILD_SEPARATE_LIBS)
    add_library(rime-gears ${librime_gears_src})
    target_link_libraries(rime-gears ${rime_gears_deps})
    add_dependencies(rime-gears ${rime_library})
    set_target_properties(rime-gears PROPERTIES VERSION ${rime_version} SOVERSION ${rime_soversion})
    if(XCODE_VERSION)
      set_target_properties(rime-gears PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS rime-gears DESTINATION ${LIB_INSTALL_DIR})
  endif()
else()
  add_library(rime-static STATIC ${rime_src})
  target_link_libraries(rime-static ${rime_deps})
  set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")
  install(TARGETS rime-static DESTINATION ${LIB_INSTALL_DIR})
  if(BUILD_SEPARATE_LIBS)
    add_library(rime-gears-static STATIC ${librime_gears_src})
    target_link_libraries(rime-gears-static ${rime_gears_deps})
    add_dependencies(rime-gears-static ${rime_library})
    set_target_properties(rime-gears-static PROPERTIES OUTPUT_NAME "rime-gears" PREFIX "lib")
    install(TARGETS rime-gears-static DESTINATION ${LIB_INSTALL_DIR})
  endif()
endif()
