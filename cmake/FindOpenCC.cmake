find_path(OPENCC_INCLUDE_DIR NAMES opencc/opencc.h)

find_library(OPENCC_LIBRARY NAMES opencc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenCC
  DEFAULT_MSG OPENCC_LIBRARY OPENCC_INCLUDE_DIR
)
if(OPENCC_FOUND AND NOT TARGET OpenCC::OpenCC)
    add_library(OpenCC::OpenCC UNKNOWN IMPORTED)
    set_target_properties(OpenCC::OpenCC PROPERTIES
        IMPORTED_LOCATION "${OPENCC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENCC_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(OPENCC_INCLUDE_DIR OPENCC_LIBRARY)
