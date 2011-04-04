project(librime)
cmake_minimum_required(VERSION 2.8)

set(LIBRIME_VERSION 0.1)
set(LIBRIME_SOVERSION 0)

if(WIN32)
  set(EXT ".exe")
endif(WIN32)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.43.0 REQUIRED COMPONENTS )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif(Boost_FOUND)

enable_testing()

find_package(Threads)
find_package(GTest REQUIRED)
if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIRS})
endif(GTEST_FOUND)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(test)

