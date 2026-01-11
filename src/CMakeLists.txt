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

function(rime_add_module name obj_target)
  add_library(${name} $<TARGET_OBJECTS:${obj_target}>)
  target_link_libraries(${name}
    ${obj_target}
    ${ARGN})
  set_target_properties(${name} PROPERTIES
    VERSION ${rime_version}
    SOVERSION ${rime_soversion}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  if(XCODE_VERSION)
    set_target_properties(${name} PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  install(TARGETS ${name} DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
endfunction()

add_library(rime-core-obj OBJECT ${rime_core_module_src})
add_library(rime-dict-obj OBJECT ${rime_dict_module_src})
add_library(rime-gears-obj OBJECT ${rime_gears_src})
add_library(rime-levers-obj OBJECT ${rime_levers_src})
if(rime_plugins_library)
  add_library(rime-plugins-obj OBJECT ${rime_plugins_src} ${rime_plugins_objs})
  set(rime_plugins_obj_list $<TARGET_OBJECTS:rime-plugins-obj>)
else()
  set(rime_plugins_obj_list ${rime_plugins_objs})
endif()

if(BUILD_SHARED_LIBS)
  set_target_properties(rime-core-obj rime-dict-obj rime-gears-obj rime-levers-obj
    PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if(rime_plugins_library)
    set_target_properties(rime-plugins-obj PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

if(ENABLE_EXTERNAL_PLUGINS)
  target_link_libraries(rime-core-obj PUBLIC dl)
endif()


target_link_libraries(rime-core-obj PUBLIC
  Boost::boost
  YamlCpp::YamlCpp
  $<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>
)

if(Glog_FOUND AND WIN32)
  target_link_libraries(rime-core-obj PUBLIC GLog::GLog)
  if(WIN32)
    # TODO: in glog v0.7.0, someone at Google forget to add dbghelp.lib to the linker library list.
    target_link_libraries(rime-core-obj PRIVATE dbghelp)
  endif()
endif()

if(MINGW)
  # fix: bcrypt for boost uuid issue
  # https://github.com/boostorg/uuid/issues/68
  target_link_libraries(rime-core-obj PRIVATE wsock32 ws2_32 bcrypt)
endif()

target_link_libraries(rime-dict-obj PUBLIC Boost::boost LevelDB::LevelDB Marisa::Marisa)
target_link_libraries(rime-gears-obj PUBLIC
  $<$<TARGET_EXISTS:Iconv::Iconv>:Iconv::Iconv>
  OpenCC::OpenCC)
target_link_libraries(rime-levers-obj PUBLIC Boost::boost)
if(rime_plugins_library)
  target_link_libraries(rime-plugins-obj PUBLIC Boost::boost ${rime_plugins_deps})
endif()

if(BUILD_SEPARATE_LIBS)
  set(rime_src $<TARGET_OBJECTS:rime-core-obj>)
  set(rime_obj_targets rime-core-obj)
else()
  set(rime_src
    $<TARGET_OBJECTS:rime-core-obj>
    $<TARGET_OBJECTS:rime-dict-obj>
    $<TARGET_OBJECTS:rime-gears-obj>
    $<TARGET_OBJECTS:rime-levers-obj>
    ${rime_plugins_obj_list})
  set(rime_obj_targets
    rime-core-obj
    rime-dict-obj
    rime-gears-obj
    rime-levers-obj)
  if(rime_plugins_library)
    list(APPEND rime_obj_targets rime-plugins-obj)
  endif()
endif()

add_library(rime ${rime_src})
target_link_libraries(rime ${rime_obj_targets})
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
  rime_add_module(rime-dict rime-dict-obj ${rime_library})
  rime_add_module(rime-gears rime-gears-obj ${rime_library} ${rime_dict_library})
  rime_add_module(rime-levers rime-levers-obj ${rime_library} ${rime_dict_library})

  if(rime_plugins_library)
    rime_add_module(rime-plugins rime-plugins-obj
      ${rime_library}
      ${rime_dict_library}
      ${rime_gears_library})
  endif()
endif()

if(BUILD_ALWAYS_STATIC_TARGET)
  if(BUILD_SEPARATE_LIBS)
    set(rime_static_src $<TARGET_OBJECTS:rime-core-obj>)
  else()
    set(rime_static_src
      $<TARGET_OBJECTS:rime-core-obj>
      $<TARGET_OBJECTS:rime-dict-obj>
      $<TARGET_OBJECTS:rime-gears-obj>
      $<TARGET_OBJECTS:rime-levers-obj>
      ${rime_plugins_obj_list})
  endif()
  add_library(rime-static STATIC ${rime_static_src})
  target_link_libraries(rime-static ${rime_obj_targets})
  set_target_properties(rime-static PROPERTIES
    OUTPUT_NAME "rime-static"
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  install(TARGETS rime-static DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
endif()
if(DEFINED RIME_SETUP_EXTRA_MODULES)
  set_property(SOURCE rime/setup.cc
    PROPERTY COMPILE_DEFINITIONS
    "RIME_EXTRA_MODULES=${RIME_SETUP_EXTRA_MODULES}")
endif()
