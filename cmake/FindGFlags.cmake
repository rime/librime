find_path(GFLAGS_INCLUDE_DIR NAMES gflags.h)

find_library(GFLAGS_LIBRARY NAMES gflags libgflags)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GFlags
  DEFAULT_MSG GFLAGS_LIBRARY GFLAGS_INCLUDE_DIR 
)
if(GFLAGS_FOUND AND NOT TARGET GFlags::GFlags)
    add_library(GFlags::GFlags UNKNOWN IMPORTED)
    set_target_properties(GFlags::GFlags PROPERTIES
        IMPORTED_LOCATION "${GFLAGS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GFLAGS_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GFLAGS_INCLUDE_DIR GFLAGS_LIBRARY)
