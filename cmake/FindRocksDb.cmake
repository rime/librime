# FindRocksDB

find_path(RocksDb_INCLUDE_DIR
  NAMES rocksdb/db.h
)

find_library(RocksDb_LIBRARY
  NAMES rocksdb
)

if(RocksDb_INCLUDE_DIR AND EXISTS "${RocksDb_INCLUDE_DIR}/rocksdb/version.h")
  foreach(ver "MAJOR" "MINOR" "PATCH")
    file(STRINGS "${RocksDb_INCLUDE_DIR}/rocksdb/version.h" ROCKSDB_VER_${ver}_LINE
      REGEX "^#define[ \t]+ROCKSDB_${ver}[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+ROCKSDB_${ver}[ \t]+([0-9]+)$"
      "\\1" ROCKSDB_VERSION_${ver} "${ROCKSDB_VER_${ver}_LINE}")
    unset(${ROCKSDB_VER_${ver}_LINE})
  endforeach()
  set(RocksDb_VERSION_STRING
    "${ROCKSDB_VERSION_MAJOR}.${ROCKSDB_VERSION_MINOR}.${ROCKSDB_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDb
  FOUND_VAR
      RocksDb_FOUND
  REQUIRED_VARS
      RocksDb_LIBRARY
      RocksDb_INCLUDE_DIR
  VERSION_VAR
      RocksDb_VERSION_STRING
)

mark_as_advanced(RocksDb_INCLUDE_DIR RocksDb_LIBRARY)
