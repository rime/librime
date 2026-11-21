find_path(LevelDb_INCLUDE_PATH NAMES leveldb/db.h)

block()
  if(LevelDb_STATIC)
    if(WIN32)
      set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  endif()
  find_library(LevelDb_LIBRARY NAMES leveldb libleveldb)
endblock()

find_package_handle_standard_args(LevelDb
  REQUIRED_VARS LevelDb_LIBRARY LevelDb_INCLUDE_PATH)

if(LevelDb_FOUND AND NOT TARGET LevelDb::LevelDb)
  add_library(LevelDb::LevelDb UNKNOWN IMPORTED)
  set_target_properties(LevelDb::LevelDb PROPERTIES
    IMPORTED_LOCATION "${LevelDb_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LevelDb_INCLUDE_PATH}"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LevelDb_INCLUDE_PATH}")
  set(LevelDb_LIBRARY LevelDb::LevelDb)
endif()
