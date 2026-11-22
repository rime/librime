# rime_api_impl.cc is included by rime_api.cc and rime_api_stdbool.cc.
set(rime_api_src
  rime_api.cc
  rime_api_stdbool.cc)

file(GLOB rime_base_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/*.cc")
file(GLOB rime_algo_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/algo/*.cc")
file(GLOB rime_config_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/config/*.cc")
file(GLOB rime_dict_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/dict/*.cc")
file(GLOB rime_gears_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/gear/*.cc")
file(GLOB rime_levers_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rime/lever/*.cc")

if(rime_plugins_library)
  set(rime_plugins_src ../plugins/plugin.cc ../plugins/plugins_module.cc)
endif()

set(rime_core_module_src
  ${rime_api_src}
  ${rime_base_src}
  ${rime_config_src})

# add rc info for windows MSVC build
if(WIN32)
  include(${PROJECT_SOURCE_DIR}/cmake/AddRCInfo.cmake)
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
if(ENABLE_EXTERNAL_PLUGINS)
  list(APPEND rime_optional_deps dl)
endif()

set(rime_core_deps
  ${Boost_LIBRARIES}
  ${YamlCpp_LIBRARY}
  Threads::Threads
  ${rime_optional_deps})
set(rime_dict_deps
  ${LevelDb_LIBRARY}
  ${Marisa_LIBRARY})
set(rime_gears_deps
  ${ICU_LIBRARIES}
  ${Opencc_LIBRARY})
set(rime_levers_deps "")

if(MINGW)
  # fix: bcrypt for boost uuid issue
  # https://github.com/boostorg/uuid/issues/68
  list(APPEND rime_core_deps wsock32 ws2_32 bcrypt)
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
  target_link_libraries(rime PUBLIC ${rime_deps})
  set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
  rime_setup_library(rime)
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:rime>
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      OPTIONAL)
  endif()

  if(BUILD_SEPARATE_LIBS)
    add_library(rime-dict ${rime_dict_module_src})
    target_link_libraries(rime-dict PUBLIC ${rime_dict_deps} ${rime_library})
    rime_setup_library(rime-dict)

    add_library(rime-gears ${rime_gears_src})
    target_link_libraries(rime-gears PUBLIC ${rime_gears_deps} ${rime_library} ${rime_dict_library})
    rime_setup_library(rime-gears)

    add_library(rime-levers ${rime_levers_src})
    target_link_libraries(rime-levers PUBLIC ${rime_levers_deps} ${rime_library} ${rime_dict_library})
    rime_setup_library(rime-levers)

    if(rime_plugins_library)
      add_library(rime-plugins ${rime_plugins_src} ${rime_plugins_objs})
      target_link_libraries(rime-plugins
        PUBLIC ${rime_plugins_deps}
        ${rime_library}
        ${rime_dict_library}
        ${rime_gears_library})
      rime_setup_library(rime-plugins)
    endif()
  endif()
else()
  add_library(rime-static STATIC ${rime_src})
  target_link_libraries(rime-static PUBLIC ${rime_deps} PRIVATE rime-build-options)
  set_target_properties(rime-static PROPERTIES
    OUTPUT_NAME "rime" PREFIX "lib"
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  install(TARGETS rime-static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(ENABLE_LOGGING)
  # Keep spdlog out of the public include/link interface.  CMake still
  # propagates it as a link-only dependency for static builds.
  target_link_libraries(${rime_library} PRIVATE spdlog::spdlog)
endif()
if(DEFINED RIME_SETUP_EXTRA_MODULES)
  target_compile_definitions(${rime_library} PRIVATE
    "RIME_EXTRA_MODULES=${RIME_SETUP_EXTRA_MODULES}")
endif()
