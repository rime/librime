find_path(YamlCpp_INCLUDE_PATH NAMES yaml-cpp/yaml.h)

find_path(YamlCpp_NEW_API NAMES yaml-cpp/node/node.h)
if(YamlCpp_INCLUDE_PATH AND NOT YamlCpp_NEW_API)
  message(FATAL_ERROR "The new yaml-cpp 0.5 API is not available.")
endif()

block()
  if(YamlCpp_STATIC)
    if(WIN32)
      set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  endif()
  find_library(YamlCpp_LIBRARY
    NAMES libyaml-cppmt libyaml-cppmtd yaml-cpp yaml-cppd)
endblock()

find_package_handle_standard_args(YamlCpp
  REQUIRED_VARS YamlCpp_LIBRARY YamlCpp_INCLUDE_PATH)

if(YamlCpp_FOUND AND NOT TARGET YamlCpp::YamlCpp)
  add_library(YamlCpp::YamlCpp UNKNOWN IMPORTED)
  set_target_properties(YamlCpp::YamlCpp PROPERTIES
    IMPORTED_LOCATION "${YamlCpp_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${YamlCpp_INCLUDE_PATH}"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${YamlCpp_INCLUDE_PATH}")
  set(YamlCpp_LIBRARY YamlCpp::YamlCpp)
endif()
