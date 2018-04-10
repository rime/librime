set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(rime)
cmake_minimum_required(VERSION 2.8.11)

set(rime_version 1.3.1)
set(rime_soversion 1)

add_definitions(-DRIME_VERSION="${rime_version}")

option(BUILD_SHARED "Build Rime as shared library" ON)
option(BUILD_STATIC "Build with dependencies as static libraries" OFF)
option(BUILD_DATA "Build data for Rime" OFF)
option(BUILD_TEST "Build and run tests" ON)
option(BUILD_SEPARATE_LIBS "Build a separate rime-gears library" OFF)
option(ENABLE_LOGGING "Enable logging with google-glog library" ON)
option(BOOST_USE_CXX11 "Boost has been built with C++11 support" OFF)
option(BOOST_USE_SIGNALS2 "Boost use signals2 instead of signals" ON)

SET(rime_data_dir "/share/rime-data" CACHE STRING "Target directory for Rime data")

if(WIN32)
  set(ext ".exe")
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${PROJECT_SOURCE_DIR}/thirdparty")

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
find_package(Boost 1.46.0 REQUIRED COMPONENTS filesystem regex signals system locale)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

if(ENABLE_LOGGING)

  find_package(Gflags)
  if(Gflags_FOUND)
    include_directories(${Gflags_INCLUDE_PATH})
  endif()

  find_package(Glog)
  if(NOT DEFINED Glog_FOUND)
    add_subdirectory(thirdparty/src/glog)
    set(Glog_LIBRARY glog)
    set(Glog_INCLUDE_PATH thirdparty/src/glog/src)
  endif()
  include_directories(${Glog_INCLUDE_PATH})
  if(Glog_STATIC)
    add_definitions(-DGOOGLE_GLOG_DLL_DECL=)
  endif()

  add_definitions(-DRIME_ENABLE_LOGGING)

endif()

find_package(Threads)

find_package(Iconv)
if (ICONV_FOUND)
  include_directories(${ICONV_INCLUDE_DIRS})
endif()

if(BUILD_TEST)
  find_package(GTest REQUIRED)
  if(GTEST_FOUND)
    enable_testing()
    include_directories(${GTEST_INCLUDE_DIRS})
  endif()
endif()

find_package(YamlCpp)
if(NOT DEFINED YamlCpp_FOUND)
  add_subdirectory(thirdparty/src/yaml-cpp)
  set(YamlCpp_LIBRARY yaml-cpp)
  set(YamlCpp_INCLUDE_PATH thirdparty/src/yaml-cpp/include)
endif()
include_directories(${YamlCpp_INCLUDE_PATH})

find_package(LevelDb)
if(NOT DEFINED LevelDb_FOUND)
   add_subdirectory(thirdparty/src/leveldb-windows)
   set(LevelDb_LIBRARY leveldb)
   set(LevelDb_INCLUDE_PATH thirdparty/src/leveldb-windows/include)
endif()
include_directories(${LevelDb_INCLUDE_PATH})

find_package(Marisa)
if(NOT DEFINED Marisa_FOUND)
   add_subdirectory(thirdparty/src/marisa-trie)
   set(Marisa_LIBRARY libmarisa)
   set(Marisa_INCLUDE_PATH thirdparty/src/marisa-trie/lib)
endif()
include_directories(${Marisa_INCLUDE_PATH})

find_package(Opencc)
if(NOT DEFINED Opencc_FOUND)
  add_subdirectory(thirdparty/src/opencc)
  set(Opencc_LIBRARY libopencc)
  file(GLOB _headers thirdparty/src/opencc/src/*.h*)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/opencc/include/opencc")
  file(COPY ${_headers} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/opencc/include/opencc")
  set(Opencc_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/opencc/include")
endif()
include_directories(${Opencc_INCLUDE_PATH})

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

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include)
link_directories(${PROJECT_SOURCE_DIR}/thirdparty/lib)

if(MSVC)
  if(MSVC10)
    # stdint.h is missing from VS2010
    include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include/msvc)
  endif()
  add_definitions("/wd4244 /wd4996")
endif()

if(UNIX)
  add_definitions("-std=c++11")
endif()

if(NOT BOOST_USE_CXX11)
  add_definitions("-DBOOST_NO_CXX11_SCOPED_ENUMS")
endif()

if(BOOST_USE_SIGNALS2)
  add_definitions("-DBOOST_SIGNALS2")
endif()

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif()

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|DragonFly")
  # uninstall target
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(bindir "${exec_prefix}/bin")
  set(libdir "${LIB_INSTALL_DIR}")
  set(pkgdatadir "${prefix}${rime_data_dir}")
  set(includedir "${CMAKE_INSTALL_PREFIX}/include")
  configure_file(
      ${PROJECT_SOURCE_DIR}/rime.pc.in
      ${PROJECT_BINARY_DIR}/rime.pc
      @ONLY)
  file(GLOB installed_header_files ${PROJECT_SOURCE_DIR}/src/*.h)
  install(FILES ${installed_header_files} DESTINATION include)
  install(FILES ${PROJECT_BINARY_DIR}/rime.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
  install(FILES cmake/RimeConfig.cmake DESTINATION share/cmake/rime)

  if(BUILD_DATA)
    file(GLOB preset_data_files ${PROJECT_SOURCE_DIR}/data/preset/*.yaml)
    install(FILES ${preset_data_files} DESTINATION ${pkgdatadir})
  endif()
endif()

if(BUILD_SHARED)
  add_definitions(-DRIME_BUILD_SHARED_LIBS)
  set(rime_library rime)
  if(BUILD_SEPARATE_LIBS)
    set(rime_gears_library rime-gears)
  endif()
else()
  set(rime_library rime-static)
  if(BUILD_SEPARATE_LIBS)
    set(rime_gears_library rime-gears-static)
  endif()
endif()

add_subdirectory(src)
add_subdirectory(tools)

if(GTEST_FOUND)
  add_subdirectory(test)
endif()
