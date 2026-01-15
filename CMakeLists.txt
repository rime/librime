cmake_minimum_required(VERSION 3.13)

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(rime VERSION 1.15.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(rime_version 1.16.0)
set(rime_soversion 1)

add_definitions(-DRIME_VERSION="${rime_version}")

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build Rime as shared library" ON)
option(BUILD_MERGED_PLUGINS "Merge plugins into one Rime library" ON)
option(WITH_STATIC_DEPS "Build with static dependency libraries" OFF)
option(BUILD_SAMPLE "Build sample Rime plugin" OFF)
option(BUILD_TEST "Build and run tests" ON)
option(BUILD_SEPARATE_LIBS "Build separate rime-* libraries" OFF)
option(BUILD_ALWAYS_STATIC_TARGET "Also build a static rime target regardless of BUILD_SHARED_LIBS" OFF)
option(ENABLE_LOGGING "Enable logging with google-glog library" ON)
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
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${PROJECT_SOURCE_DIR}")

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address -lasan)
endif()


find_package(Boost CONFIG 1.74.0 REQUIRED COMPONENTS regex)

add_definitions(-DBOOST_DLL_USE_STD_FS)
set(Boost_USE_STATIC_LIBS ${WITH_STATIC_DEPS})
set(Boost_USE_MULTITHREADED ON)

if(ENABLE_LOGGING)
  find_package(GFlags CONFIG QUIET)
  find_package(GLog CONFIG REQUIRED)
  if(Glog_FOUND)
    if(WIN32)
      add_compile_definitions("GLOG_DEPRECATED=__declspec(deprecated)")
      add_compile_definitions(GLOG_NO_ABBREVIATED_SEVERITIES)
    else()
      add_compile_definitions("GLOG_DEPRECATED=__attribute__((deprecated))")
    endif()
    if(Glog_STATIC)
      add_compile_definitions(GLOG_EXPORT=)
      add_compile_definitions(GLOG_NO_EXPORT=)
    else()
      add_compile_definitions("GLOG_EXPORT=__attribute__((visibility(\"default\")))")
      add_compile_definitions("GLOG_NO_EXPORT=__attribute__((visibility(\"default\")))")
    endif()

    if(WITH_STATIC_DEPS)
      add_definitions(-DGOOGLE_GLOG_DLL_DECL=)
    endif()

    set(RIME_ENABLE_LOGGING 1)

    if(ALSO_LOG_TO_STDERR)
      set(RIME_ALSO_LOG_TO_STDERR 1)
    endif()
  endif()
endif()

find_package(Iconv)
find_package(Threads)

if(NOT ENABLE_THREADING)
  add_definitions(-DRIME_NO_THREADING)
endif()

if(NOT ENABLE_TIMESTAMP)
  add_definitions(-DRIME_NO_TIMESTAMP)
endif()

if(BUILD_TEST)
  find_package(GTest CONFIG REQUIRED)
  if(GTEST_FOUND)
    enable_testing()
  endif()
endif()

find_package(yaml-cpp CONFIG REQUIRED)
if(WITH_STATIC_DEPS)
  add_definitions(-DYAML_CPP_STATIC_DEFINE)
endif()

find_package(leveldb CONFIG REQUIRED)

# debian 12 used in docker CI and ubuntu 24 Linux in CI do not have Marisa cmake config file
find_package(Marisa CONFIG QUIET)
if(NOT Marisa_FOUND)
  find_package(Marisa MODULE REQUIRED)
endif()

# debian 12 used in docker CI do not have OpenCC cmake config file
find_package(OpenCC CONFIG QUIET)
if(NOT OpenCC_FOUND)
  find_package(OpenCC MODULE REQUIRED)
endif()
if(WITH_STATIC_DEPS)
  add_definitions(-DOpencc_BUILT_AS_STATIC)
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

if(MSVC)
  # https://stackoverflow.com/a/31264946
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  add_definitions("/wd4244 /wd4996")

  # large address aware
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
endif()

if(NOT CMAKE_BUILD_PARALLEL_LEVEL)
  include(ProcessorCount)
  ProcessorCount(N)
  if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
  endif()
endif()

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

# Android doesn't support PkgConfig yet
if(NOT ANDROID)
  configure_file(
    ${PROJECT_SOURCE_DIR}/rime.pc.in
    ${PROJECT_BINARY_DIR}/rime.pc
    @ONLY)
  install(FILES ${PROJECT_BINARY_DIR}/rime.pc
    DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig)
endif()

install(FILES cmake/RimeConfig.cmake
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/cmake/rime)

file(GLOB rime_public_header_files ${PROJECT_SOURCE_DIR}/src/*.h)
list(FILTER rime_public_header_files EXCLUDE REGEX .*_impl\.h$)
install(FILES ${rime_public_header_files}
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
if(INSTALL_PRIVATE_HEADERS)
  file(GLOB rime_private_header_files
    ${PROJECT_SOURCE_DIR}/src/rime/*.h
    ${PROJECT_BINARY_DIR}/src/rime/*.h)
  install(FILES ${rime_private_header_files}
    DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/rime)
  foreach(rime_private_header_files_dir algo config dict gear lever)
    file(GLOB rime_private_header_files
      ${PROJECT_SOURCE_DIR}/src/rime/${rime_private_header_files_dir}/*.h)
    install(FILES ${rime_private_header_files}
      DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/rime/${rime_private_header_files_dir})
  endforeach()
endif()

if(BUILD_SHARED_LIBS)
  add_definitions(-DRIME_BUILD_SHARED_LIBS)
endif()
set(rime_library rime)
if(BUILD_SEPARATE_LIBS)
  set(rime_dict_library rime-dict)
  set(rime_gears_library rime-gears)
  set(rime_levers_library rime-levers)
endif()

add_subdirectory(plugins)
message(STATUS "rime_plugins_libs: ${rime_plugins_deps}")
message(STATUS "rime_plugins_modules: ${rime_plugins_modules}")
set(list "")
foreach(mod ${rime_plugins_modules})
  set(list "${list},Q(${mod})")
endforeach()
set(RIME_SETUP_EXTRA_MODULES "${list}")
if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS AND rime_plugins_objs)
  set(rime_plugins_library rime-plugins)
endif()

add_subdirectory(src)

if(BUILD_SHARED_LIBS)
  add_subdirectory(tools)

  if(BUILD_TEST)
    add_subdirectory(test)
  endif()

  if(BUILD_SAMPLE)
    add_subdirectory(sample)
  endif()
endif()
