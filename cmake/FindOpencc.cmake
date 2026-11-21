find_path(Opencc_INCLUDE_PATH NAMES opencc/opencc.h)

block()
  if(Opencc_STATIC)
    if(WIN32)
      set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  endif()
  find_library(Opencc_LIBRARY NAMES opencc)
endblock()

# Locate the installed opencc config dict directory (e.g. t2s.json), which
# lives under <prefix>/share/opencc alongside <prefix>/include.
if(Opencc_INCLUDE_PATH)
  get_filename_component(_opencc_prefix "${Opencc_INCLUDE_PATH}" DIRECTORY)
  find_path(Opencc_DICT_DIR
    NAMES t2s.json
    PATHS "${_opencc_prefix}/share/opencc"
    NO_DEFAULT_PATH)
  unset(_opencc_prefix)
endif()

find_package_handle_standard_args(Opencc
  REQUIRED_VARS Opencc_LIBRARY Opencc_INCLUDE_PATH)

if(Opencc_FOUND)
  if(NOT Opencc_FIND_QUIETLY AND Opencc_DICT_DIR)
    message(STATUS "Found OpenCC dict dir: ${Opencc_DICT_DIR}")
  endif()
  if(NOT TARGET Opencc::Opencc)
    add_library(Opencc::Opencc UNKNOWN IMPORTED)
    set_target_properties(Opencc::Opencc PROPERTIES
      IMPORTED_LOCATION "${Opencc_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${Opencc_INCLUDE_PATH}"
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${Opencc_INCLUDE_PATH}")
    set(Opencc_LIBRARY Opencc::Opencc)
  endif()
endif()
