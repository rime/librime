project(librime)
cmake_minimum_required(VERSION 2.8)

set(LIBRIME_VERSION 0.1)
set(LIBRIME_SOVERSION 0)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

find_package(GTEST)
if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIR})
  add_subdirectory(test)
else(GTEST_FOUND)
  message("Unit tests will be skipped due to the absence of GTest.")
endif(GTEST_FOUND)

