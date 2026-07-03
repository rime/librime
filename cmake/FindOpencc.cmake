set(_opencc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

find_path(Opencc_INCLUDE_PATH opencc/opencc.h)

if (Opencc_STATIC)
  if (WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else (WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  endif (WIN32)
endif (Opencc_STATIC)
find_library(Opencc_LIBRARY NAMES opencc)
if(Opencc_INCLUDE_PATH AND Opencc_LIBRARY)
  set(Opencc_FOUND TRUE)
endif(Opencc_INCLUDE_PATH AND Opencc_LIBRARY)

# Locate the installed opencc config dict directory (e.g. t2s.json),
# which lives under <prefix>/share/opencc alongside <prefix>/include.
if(Opencc_INCLUDE_PATH)
  get_filename_component(_opencc_prefix "${Opencc_INCLUDE_PATH}" DIRECTORY)
  find_path(Opencc_DICT_DIR
    NAMES t2s.json
    PATHS "${_opencc_prefix}/share/opencc"
    NO_DEFAULT_PATH)
  unset(_opencc_prefix)
endif()

if(Opencc_FOUND)
  if(NOT Opencc_FIND_QUIETLY)
    message(STATUS "Found opencc: ${Opencc_LIBRARY}")
    if(Opencc_DICT_DIR)
      message(STATUS "Found opencc dict dir: ${Opencc_DICT_DIR}")
    endif()
  endif(NOT Opencc_FIND_QUIETLY)
else(Opencc_FOUND)
  if(Opencc_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find opencc library.")
  endif(Opencc_FIND_REQUIRED)
endif(Opencc_FOUND)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_opencc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
