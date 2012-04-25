set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(rime)
cmake_minimum_required(VERSION 2.8)

set(LIBRIME_VERSION 0.9)
set(LIBRIME_SOVERSION 0)

option(BUILD_STATIC "Build static version of Rime" ON)

if(WIN32)
  set(EXT ".exe")
endif(WIN32)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(KyotoCabinet_STATIC ${BUILD_STATIC})
set(Opencc_STATIC ${BUILD_STATIC})
set(YamlCpp_STATIC ${BUILD_STATIC})

set(Boost_USE_MULTITHREADED ON)
if(MSVC)
  set(Boost_USE_STATIC_RUNTIME ON)
endif(MSVC)
find_package(Boost 1.43.0 REQUIRED COMPONENTS filesystem regex signals system thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif(Boost_FOUND)

find_package(Threads)
find_package(GTest)
if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIRS})
endif(GTEST_FOUND)

find_package(YamlCpp REQUIRED)
if(YamlCpp_FOUND)
  include_directories(${YamlCpp_INCLUDE_PATH})
endif(YamlCpp_FOUND)

find_package(ZLIB)
find_package(KyotoCabinet REQUIRED)
if(KyotoCabinet_FOUND)
  include_directories(${KyotoCabinet_INCLUDE_PATH})
endif(KyotoCabinet_FOUND)

find_package(Opencc REQUIRED)
if(Opencc_FOUND)
  include_directories(${Opencc_INCLUDE_PATH})
endif(Opencc_FOUND)

find_path(X11Keysym X11/keysym.h)
if(X11Keysym)
  message(STATUS "Found X11/keysym.h at ${X11Keysym}")
  include_directories(${X11Keysym})
else(X11Keysym)
  message(WARNING "X11/keysym.h not found.")
endif(X11Keysym)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include)
link_directories(${PROJECT_SOURCE_DIR}/thirdparty/lib)
if(MSVC)
  include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include/msvc)
  add_definitions("/wd4244 /wd4996")
endif(MSVC)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()


if (NOT BUILD_STATIC)
  # uninstall target
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
  set (prefix "${CMAKE_INSTALL_PREFIX}")
  set (exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set (libdir "${LIB_INSTALL_DIR}")
  set (includedir "${CMAKE_INSTALL_PREFIX}/include")
  configure_file(${PROJECT_SOURCE_DIR}/rime.pc.in ${PROJECT_BINARY_DIR}/rime.pc)
  install(FILES include/rime_api.h DESTINATION include)
  install(FILES ${PROJECT_BINARY_DIR}/rime.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
  install(FILES cmake/RimeConfig.cmake DESTINATION share/cmake/rime)
endif (NOT BUILD_STATIC)

add_subdirectory(src)
add_subdirectory(tools)
if(GTEST_FOUND)
  add_subdirectory(test)
endif(GTEST_FOUND)
