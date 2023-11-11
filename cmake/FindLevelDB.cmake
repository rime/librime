find_path(LEVELDB_INCLUDE_DIR NAMES leveldb/db.h)

find_library(LEVELDB_LIBRARY NAMES leveldb libleveldb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LevelDB
  DEFAULT_MSG LEVELDB_LIBRARY LEVELDB_INCLUDE_DIR
)
if(LEVELDB_FOUND AND NOT TARGET LevelDB::LevelDB)
    add_library(LevelDB::LevelDB UNKNOWN IMPORTED)
    set_target_properties(LevelDB::LevelDB PROPERTIES
        IMPORTED_LOCATION "${LEVELDB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LEVELDB_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LEVELDB_INCLUDE_DIR LEVELDB_LIBRARY)
