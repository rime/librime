set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

cmake_minimum_required(VERSION 3.12)
project(rime)
set(CMAKE_CXX_STANDARD 14)

set(rime_version 1.9.0)
set(rime_soversion 1)

add_definitions(-DRIME_VERSION="${rime_version}")

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build Rime as shared library" ON)
option(BUILD_MERGED_PLUGINS "Merge plugins into one Rime library" ON)
option(BUILD_STATIC "Build with dependencies as static libraries" OFF)
option(BUILD_DATA "Build data for Rime" OFF)
option(BUILD_SAMPLE "Build sample Rime plugin" OFF)
option(BUILD_TEST "Build and run tests" ON)
option(BUILD_SEPARATE_LIBS "Build separate rime-* libraries" OFF)
option(ENABLE_LOGGING "Enable logging with google-glog library" ON)
option(BOOST_USE_SIGNALS2 "Boost use signals2 instead of signals" ON)
option(ENABLE_ASAN "Enable Address Sanitizer (Unix Only)" OFF)
option(INSTALL_PRIVATE_HEADERS "Install private headers (usually needed for externally built Rime plugins)" OFF)
option(ENABLE_EXTERNAL_PLUGINS "Enable loading of externally built Rime plugins (from directory set by RIME_PLUGINS_DIR variable)" OFF)
option(ENABLE_THREADING "Enable threading for deployer" ON)

set(RIME_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/rime-data" CACHE STRING "Target directory for Rime data")
set(RIME_PLUGINS_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/rime-plugins" CACHE STRING "Target directory for externally built Rime plugins")

if(WIN32)
  set(ext ".exe")
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${PROJECT_SOURCE_DIR}")

if (ENABLE_ASAN)
  set(asan_cflags "-fsanitize=address -fno-omit-frame-pointer")
  set(asan_lflags "-fsanitize=address -lasan")
  set(CMAKE_C_FLAGS "${asan_cflags} ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${asan_cflags} ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${asan_lflags} ${CMAKE_EXE_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${asan_lflags} ${CMAKE_SHARED_LINKER_FLAGS}")
endif()

set(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(Gflags_STATIC ${BUILD_STATIC})
set(Glog_STATIC ${BUILD_STATIC})
set(LevelDb_STATIC ${BUILD_STATIC})
set(Marisa_STATIC ${BUILD_STATIC})
set(Opencc_STATIC ${BUILD_STATIC})
set(YamlCpp_STATIC ${BUILD_STATIC})

set(Boost_USE_MULTITHREADED ON)
if(MSVC)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

set(BOOST_COMPONENTS filesystem regex system)

if(BOOST_USE_SIGNALS2)
  set(RIME_BOOST_SIGNALS2 1)
else()
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} signals)
endif()

find_package(Boost 1.74.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

if(ENABLE_LOGGING)

  find_package(Gflags)
  if(Gflags_FOUND)
    include_directories(${Gflags_INCLUDE_PATH})
  endif()

  find_package(Glog REQUIRED)
  if(Glog_FOUND)
    include_directories(${Glog_INCLUDE_PATH})
  endif()
  if(Glog_STATIC)
    add_definitions(-DGOOGLE_GLOG_DLL_DECL=)
  endif()

  set(RIME_ENABLE_LOGGING 1)

endif()

find_package(Threads)

if(NOT ENABLE_THREADING)
  add_definitions(-DRIME_NO_THREADING)
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
  add_definitions(-DYAML_CPP_STATIC_DEFINE)
endif()

find_package(LevelDb REQUIRED)
if(LevelDb_FOUND)
    include_directories(${LevelDb_INCLUDE_PATH})
endif()

find_package(Marisa REQUIRED)
if(Marisa_FOUND)
  include_directories(${Marisa_INCLUDE_PATH})
endif()

find_package(Opencc REQUIRED)
if(Opencc_FOUND)
include_directories(${Opencc_INCLUDE_PATH})
endif()
if(Opencc_STATIC)
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
endif()

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if (NOT CMAKE_BUILD_PARALLEL_LEVEL)
  include(ProcessorCount)
  ProcessorCount(N)
  if (NOT N EQUAL 0)
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

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|DragonFly|GNU" OR MINGW)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(bindir "${CMAKE_INSTALL_FULL_BINDIR}")
  set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
  set(pkgdatadir "${RIME_DATA_DIR}")
  set(pluginsdir "${RIME_PLUGINS_DIR}")
  set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
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

if(BUILD_DATA)
  file(GLOB rime_preset_data_files ${PROJECT_SOURCE_DIR}/data/preset/*.yaml)
  install(FILES ${rime_preset_data_files} DESTINATION ${RIME_DATA_DIR})
endif()

if(BUILD_SHARED_LIBS)
  add_definitions(-DRIME_BUILD_SHARED_LIBS)
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
set(list "")
foreach(mod ${rime_plugins_modules})
  set(list "${list},Q(${mod})")
endforeach()
add_definitions(-DRIME_EXTRA_MODULES=${list})
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
