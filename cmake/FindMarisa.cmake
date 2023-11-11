find_path(MARISA_INCLUDE_DIR NAMES marisa.h)

find_library(MARISA_LIBRARY NAMES marisa libmarisa)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Marisa
  DEFAULT_MSG MARISA_LIBRARY MARISA_INCLUDE_DIR
)
if(MARISA_FOUND AND NOT TARGET Marisa::Marisa)
    add_library(Marisa::Marisa UNKNOWN IMPORTED)
    set_target_properties(Marisa::Marisa PROPERTIES
        IMPORTED_LOCATION "${MARISA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MARISA_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(MARISA_INCLUDE_DIR MARISA_LIBRARY)
