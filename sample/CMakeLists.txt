project(rime-sample)
cmake_minimum_required(VERSION 2.8)

set(SAMPLE_VERSION 1.0.0)
set(SAMPLE_SOVERSION 1)

option(BUILD_SHARED_LIBS "Build Rime as shared library" ON)
option(BUILD_STATIC "Build sample using static libraries" OFF)
option(BUILD_TEST "Build and run tests" ON)

if(WIN32)
  message(FATAL_ERROR "Plugins are not supported under Windows.")
endif(WIN32)

set(RIME_SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)
set(CMAKE_MODULE_PATH ${RIME_SOURCE_DIR}/cmake)

set(Gflags_STATIC ${BUILD_STATIC})
set(Glog_STATIC ${BUILD_STATIC})

find_package(Rime REQUIRED)
if(Rime_FOUND)
  include_directories(${Rime_INCLUDE_DIR})
endif(Rime_FOUND)

find_package(Gflags)
if(Gflags_FOUND)
  include_directories(${Gflags_INCLUDE_PATH})
endif(Gflags_FOUND)

find_package(Glog REQUIRED)
if(Glog_FOUND)
  include_directories(${Glog_INCLUDE_PATH})
endif(Glog_FOUND)

if(BUILD_TEST)
  find_package(GTest REQUIRED)
if(GTEST_FOUND)
  enable_testing()
  include_directories(${GTEST_INCLUDE_DIRS})
endif(GTEST_FOUND)
endif(BUILD_TEST)

include_directories(${RIME_SOURCE_DIR}/include)
#include_directories(${RIME_SOURCE_DIR}/thirdparty/include)
#link_directories(${RIME_SOURCE_DIR}/thirdparty/lib)
if(UNIX)
  add_definitions("-std=c++11")
endif(UNIX)

add_subdirectory(src)
add_subdirectory(tools)
if(GTEST_FOUND)
  add_subdirectory(test)
endif(GTEST_FOUND)
