cmake_minimum_required(VERSION 3.25)

# Preserve rules supplied by a parent project: librime's MSVC/MinGW runtime
# settings (notably /MT for static builds) must still be applied alongside them.
list(APPEND CMAKE_USER_MAKE_RULES_OVERRIDE
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake")
list(APPEND CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake")

project(rime LANGUAGES C CXX)

set(rime_version 1.18.0)
set(rime_soversion 1)
add_compile_definitions(RIME_VERSION="${rime_version}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build Rime as shared library" ON)
option(BUILD_MERGED_PLUGINS "Merge plugins into one Rime library" ON)
option(BUILD_STATIC "Build with dependencies as static libraries" OFF)
option(BUILD_DATA "Build data for Rime" OFF)
option(BUILD_SAMPLE "Build sample Rime plugin" OFF)
option(BUILD_TEST "Build and run tests" ON)
option(BUILD_SEPARATE_LIBS "Build separate rime-* libraries" OFF)
option(ENABLE_LOGGING "Enable logging with spdlog library" ON)
option(ALSO_LOG_TO_STDERR "Log to stderr as well as log file" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer (Unix Only)" OFF)
option(INSTALL_PRIVATE_HEADERS "Install private headers (usually needed for externally built Rime plugins)" OFF)
option(ENABLE_EXTERNAL_PLUGINS "Enable loading of externally built Rime plugins (from directory set by RIME_PLUGINS_DIR variable)" OFF)
option(ENABLE_THREADING "Enable threading for deployer" ON)
option(ENABLE_TIMESTAMP "Embed timestamp to schema artifacts" ON)

set(RIME_DATA_DIR "rime-data" CACHE STRING "Target directory for Rime data")
set(RIME_PLUGINS_DIR "rime-plugins" CACHE STRING "Target directory for externally built Rime plugins")

if(WIN32)
  set(ext ".exe")
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${PROJECT_SOURCE_DIR}")

add_library(rime-build-options INTERFACE)
target_compile_features(rime-build-options INTERFACE cxx_std_17)

if(ENABLE_ASAN)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # ASan needs whole-build instrumentation: plugin object libraries and
    # bundled dependencies are not linked to rime-build-options, so attach the
    # flags directory-wide instead of to that interface target.
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    # The compiler driver links the right runtime; no explicit -lasan needed.
    add_link_options(-fsanitize=address)
  else()
    message(WARNING "ENABLE_ASAN is only supported with GCC or Clang; ignoring.")
  endif()
endif()

if(MSVC)
  target_compile_options(rime-build-options INTERFACE
    /wd4244
    /wd4996
    /Zc:__cplusplus
    $<$<CONFIG:Release>:/Zi>)
  # https://stackoverflow.com/a/31264946
  target_link_options(rime-build-options INTERFACE
    /LARGEADDRESSAWARE
    $<$<CONFIG:Release>:/DEBUG>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>)
elseif(NOT WIN32)
  target_compile_options(rime-build-options INTERFACE
    -Wall
    -Wextra
    -Wundef
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Werror=return-type
    -Werror=implicit-fallthrough
    -Werror=format=2
    -Werror=delete-non-virtual-dtor)
endif()

function(rime_setup_library target)
  target_link_libraries(${target} PRIVATE rime-build-options)
  set_target_properties(${target} PROPERTIES
    VERSION ${rime_version}
    SOVERSION ${rime_soversion}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  if(XCODE_VERSION)
    set_target_properties(${target} PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  install(TARGETS ${target}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
endfunction()

function(rime_setup_executable target output_dir)
  target_link_libraries(${target} PRIVATE rime-build-options)
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${output_dir}")
endfunction()

set(LevelDb_STATIC ${BUILD_STATIC})
set(Marisa_STATIC ${BUILD_STATIC})
set(Opencc_STATIC ${BUILD_STATIC})
set(YamlCpp_STATIC ${BUILD_STATIC})

find_package(Boost 1.83.0 QUIET CONFIG)
if(NOT Boost_FOUND)
  find_package(Boost 1.83.0 REQUIRED MODULE)
endif()
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
add_compile_definitions(BOOST_DLL_USE_STD_FS)

if(ENABLE_LOGGING)
  set(RIME_ENABLE_LOGGING 1)

  if (ALSO_LOG_TO_STDERR)
     set(RIME_ALSO_LOG_TO_STDERR 1)
  endif()
endif()

find_package(Threads REQUIRED)

if(ENABLE_LOGGING)
  # Always use the bundled, pinned spdlog so that the library is linked
  # statically and no new runtime dependency is introduced.
  if(NOT TARGET spdlog::spdlog)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/spdlog/CMakeLists.txt")
      message(FATAL_ERROR
        "Bundled spdlog is required for static linking. "
        "Initialize it with: git submodule update --init deps/spdlog")
    endif()

    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
    set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)

    # SPDLOG_BUILD_SHARED is not enough when the parent project builds
    # shared libraries: spdlog also looks at BUILD_SHARED_LIBS.
    set(_rime_build_shared_libs ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(deps/spdlog EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${_rime_build_shared_libs})
  endif()

  if(NOT TARGET spdlog::spdlog)
    message(FATAL_ERROR "spdlog target not found")
  endif()
endif()

if(NOT ENABLE_THREADING)
  add_compile_definitions(RIME_NO_THREADING)
endif()

if(NOT ENABLE_TIMESTAMP)
  add_compile_definitions(RIME_NO_TIMESTAMP)
endif()

if(BUILD_TEST)
  find_package(GTest REQUIRED)
  if(GTEST_FOUND)
    enable_testing()
    include_directories(${GTEST_INCLUDE_DIRS})
  endif()
endif()

find_package(YamlCpp REQUIRED)
if(YamlCpp_FOUND)
  include_directories(${YamlCpp_INCLUDE_PATH})
endif()
if(YamlCpp_STATIC)
  add_compile_definitions(YAML_CPP_STATIC_DEFINE)
endif()

find_package(LevelDb REQUIRED)
if(LevelDb_FOUND AND LevelDb_INCLUDE_PATH)
    include_directories(SYSTEM ${LevelDb_INCLUDE_PATH})
endif()

# Don't let leveldb's PUBLIC -Werror turn librime warnings into errors.
if(TARGET leveldb)
  set_target_properties(leveldb PROPERTIES SYSTEM TRUE)
  target_compile_options(leveldb INTERFACE -Wno-error)
endif()

find_package(Marisa REQUIRED)
if(Marisa_FOUND)
  include_directories(${Marisa_INCLUDE_PATH})
endif()

find_package(Opencc REQUIRED)
if(Opencc_FOUND)
include_directories(SYSTEM ${Opencc_INCLUDE_PATH})
endif()
if(TARGET libopencc)
  set_target_properties(libopencc PROPERTIES SYSTEM TRUE)
endif()
if(Opencc_STATIC)
  add_compile_definitions(Opencc_BUILT_AS_STATIC)
endif()

find_path(X11Keysym X11/keysym.h)
if(X11Keysym)
  message(STATUS "Found X11/keysym.h at ${X11Keysym}")
  include_directories(${X11Keysym})
else()
  message(WARNING "X11/keysym.h not found.")
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/src/rime/build_config.h.in"
  "${PROJECT_BINARY_DIR}/src/rime/build_config.h")

include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)

# keep these variables lest some Rime plugin's cmake file is still using them {
if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
endif()
# }

# remove target
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(remove
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|OpenBSD|DragonFly|GNU|Darwin" OR MINGW)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(bindir "${CMAKE_INSTALL_FULL_BINDIR}")
  set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
  set(pkgdatadir "${CMAKE_INSTALL_FULL_DATADIR}/${RIME_DATA_DIR}")
  set(pluginsdir "${CMAKE_INSTALL_FULL_LIBDIR}/${RIME_PLUGINS_DIR}")
  set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  configure_file(
      ${PROJECT_SOURCE_DIR}/rime.pc.in
      ${PROJECT_BINARY_DIR}/rime.pc
      @ONLY)
  install(FILES ${PROJECT_BINARY_DIR}/rime.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

install(FILES cmake/RimeConfig.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/rime)

file(GLOB rime_public_header_files ${PROJECT_SOURCE_DIR}/src/*.h)
list(FILTER rime_public_header_files EXCLUDE REGEX .*_impl\.h$)
install(FILES ${rime_public_header_files}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(INSTALL_PRIVATE_HEADERS)
  file(GLOB rime_private_header_files
    ${PROJECT_SOURCE_DIR}/src/rime/*.h
    ${PROJECT_BINARY_DIR}/src/rime/*.h)
  install(FILES ${rime_private_header_files}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rime)
  foreach(rime_private_header_files_dir algo config dict gear lever)
    file(GLOB rime_private_header_files
      ${PROJECT_SOURCE_DIR}/src/rime/${rime_private_header_files_dir}/*.h)
    install(FILES ${rime_private_header_files}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rime/${rime_private_header_files_dir})
  endforeach()
  # rime/dict/prism.h includes the bundled darts-clone header
  install(FILES ${PROJECT_SOURCE_DIR}/include/darts.h
    DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/rime/dict)
endif()

if(BUILD_DATA)
  file(GLOB rime_preset_data_files ${PROJECT_SOURCE_DIR}/data/preset/*.yaml)
  install(FILES ${rime_preset_data_files} DESTINATION ${CMAKE_INSTALL_DATADIR}/${RIME_DATA_DIR})
endif()

if(BUILD_SHARED_LIBS)
  add_compile_definitions(RIME_BUILD_SHARED_LIBS)
  set(rime_library rime)
  if(BUILD_SEPARATE_LIBS)
    set(rime_dict_library rime-dict)
    set(rime_gears_library rime-gears)
    set(rime_levers_library rime-levers)
  endif()
else()
  set(rime_library rime-static)
endif()

add_subdirectory(plugins)
message(STATUS "rime_plugins_libs: ${rime_plugins_deps}")
message(STATUS "rime_plugins_modules: ${rime_plugins_modules}")
if(rime_plugins_modules)
  string(REPLACE ";" ")(" _rime_modules "${rime_plugins_modules}")
  set(RIME_SETUP_EXTRA_MODULES "(${_rime_modules})")
else()
  set(RIME_SETUP_EXTRA_MODULES "")
endif()
if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS AND rime_plugins_objs)
  set(rime_plugins_library rime-plugins)
endif()

add_subdirectory(src)

if(BUILD_SHARED_LIBS)
  add_subdirectory(tools)

  if(BUILD_TEST)
    add_subdirectory(test)
  endif()

  if (BUILD_SAMPLE)
    add_subdirectory(sample)
  endif()
endif()
