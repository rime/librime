find_path(Marisa_INCLUDE_PATH NAMES marisa.h)

block()
  if(Marisa_STATIC)
    if(WIN32)
      set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  endif()
  find_library(Marisa_LIBRARY NAMES marisa libmarisa)
endblock()

find_package_handle_standard_args(Marisa
  REQUIRED_VARS Marisa_LIBRARY Marisa_INCLUDE_PATH)

if(Marisa_FOUND AND NOT TARGET Marisa::Marisa)
  add_library(Marisa::Marisa UNKNOWN IMPORTED)
  set_target_properties(Marisa::Marisa PROPERTIES
    IMPORTED_LOCATION "${Marisa_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Marisa_INCLUDE_PATH}"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${Marisa_INCLUDE_PATH}")
  set(Marisa_LIBRARY Marisa::Marisa)
endif()
