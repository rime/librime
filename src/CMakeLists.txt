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

function(rime_add_module module_name)
  set(options)
  set(one_value_args)
  set(multi_value_args SOURCES PUBLIC_DEPS PRIVATE_DEPS LIB_DEPS)
  cmake_parse_arguments(MODULE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(NOT MODULE_SOURCES)
    message(FATAL_ERROR "rime_add_module(${module_name}) requires SOURCES")
  endif()

  set(obj_target "rime-${module_name}-obj")
  add_library(${obj_target} OBJECT ${MODULE_SOURCES})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${obj_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(${obj_target} PRIVATE RIME_EXPORTS)
  endif()
  if(MODULE_PUBLIC_DEPS)
    target_link_libraries(${obj_target} PUBLIC ${MODULE_PUBLIC_DEPS})
  endif()
  if(MODULE_PRIVATE_DEPS)
    target_link_libraries(${obj_target} PRIVATE ${MODULE_PRIVATE_DEPS})
  endif()
  set(build_lib OFF)
  if("${module_name}" STREQUAL "core")
    set(build_lib ON)
  elseif(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS)
    set(build_lib ON)
  endif()

  if(build_lib)
    set(lib_target "rime-${module_name}")

    add_library(${lib_target})
    target_sources(${lib_target} PRIVATE $<TARGET_OBJECTS:${obj_target}>)
    if(MODULE_PUBLIC_DEPS)
      target_link_libraries(${lib_target} PUBLIC ${MODULE_PUBLIC_DEPS})
    endif()
    if(MODULE_PRIVATE_DEPS)
      target_link_libraries(${lib_target} PRIVATE ${MODULE_PRIVATE_DEPS})
    endif()
    if(MODULE_LIB_DEPS)
      target_link_libraries(${lib_target} PUBLIC ${MODULE_LIB_DEPS})
    endif()
    if(NOT "${module_name}" STREQUAL "core")
      if(BUILD_SHARED_LIBS)
        target_link_libraries(${lib_target} PUBLIC rime-core)
      else()
        target_link_libraries(${lib_target} PUBLIC rime)
      endif()
    endif()

    set_target_properties(${lib_target} PROPERTIES
      VERSION ${rime_version}
      SOVERSION ${rime_soversion}
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    if(XCODE_VERSION)
      set_target_properties(${lib_target} PROPERTIES INSTALL_NAME_DIR "@rpath")
    endif()
    install(TARGETS ${lib_target} DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
  endif()

endfunction()

set(rime_core_public_deps
  Boost::boost
  Boost::regex
  yaml-cpp::yaml-cpp
  $<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>
  $<$<BOOL:${ENABLE_EXTERNAL_PLUGINS}>:dl>
  $<$<TARGET_EXISTS:Glog::Glog>:GLog::GLog>)
set(rime_core_private_deps
  $<$<TARGET_EXISTS:Glog::Glog>:dbghelp>
  $<$<BOOL:${MINGW}>:wsock32>
  $<$<BOOL:${MINGW}>:ws2_32>
  $<$<BOOL:${MINGW}>:bcrypt>)

rime_add_module(
  core
  SOURCES ${rime_core_module_src}
  PUBLIC_DEPS ${rime_core_public_deps}
  PRIVATE_DEPS ${rime_core_private_deps})

rime_add_module(
  dict
  SOURCES ${rime_dict_module_src}
  PUBLIC_DEPS Boost::boost leveldb::leveldb Marisa::marisa)

rime_add_module(
  gears
  SOURCES ${rime_gears_src}
  PUBLIC_DEPS Boost::boost Boost::regex $<$<TARGET_EXISTS:Iconv::Iconv>:Iconv::Iconv> OpenCC::OpenCC
  LIB_DEPS rime-dict)

rime_add_module(
  levers
  SOURCES ${rime_levers_src}
  PUBLIC_DEPS Boost::boost
  LIB_DEPS rime-dict)

if(rime_plugins_library)
  rime_add_module(
    plugins
    SOURCES ${rime_plugins_src} ${rime_plugins_objs}
    PUBLIC_DEPS ${rime_plugins_deps}
    LIB_DEPS rime-dict rime-gears)
endif()

set_target_properties(rime-core PROPERTIES OUTPUT_NAME "rime")
if(BUILD_SHARED_LIBS)
  set_target_properties(rime-core PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
endif()

if(NOT BUILD_SEPARATE_LIBS)
  target_sources(rime-core PRIVATE
    $<TARGET_OBJECTS:rime-dict-obj>
    $<TARGET_OBJECTS:rime-gears-obj>
    $<TARGET_OBJECTS:rime-levers-obj>)
  target_link_libraries(rime-core PUBLIC
    rime-dict-obj
    rime-gears-obj
    rime-levers-obj)
  if(rime_plugins_library)
    target_sources(rime-core PRIVATE $<TARGET_OBJECTS:rime-plugins-obj>)
    target_link_libraries(rime-core PUBLIC rime-plugins-obj)
  elseif(rime_plugins_objs)
    target_sources(rime-core PRIVATE ${rime_plugins_objs})
  endif()
endif()


if(NOT BUILD_SEPARATE_LIBS AND rime_plugins_deps AND NOT rime_plugins_library)
  target_link_libraries(rime-core PUBLIC ${rime_plugins_deps})
endif()


if(MSVC AND BUILD_SHARED_LIBS)
  install(FILES $<TARGET_PDB_FILE:rime-core>
    DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    OPTIONAL)
endif()

# Aggregate target for consumers that want all modules.
add_library(rime INTERFACE)
if(TARGET rime-core)
  target_link_libraries(rime INTERFACE rime-core)
endif()

if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS)
  target_link_libraries(rime INTERFACE rime-dict rime-gears rime-levers)
endif()

if(DEFINED RIME_SETUP_EXTRA_MODULES)
  set_property(SOURCE rime/setup.cc
    PROPERTY COMPILE_DEFINITIONS
    "RIME_EXTRA_MODULES=${RIME_SETUP_EXTRA_MODULES}")
endif()
