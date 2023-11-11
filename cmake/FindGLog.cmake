find_path(GLOG_INCLUDE_DIR NAMES glog/logging.h)

find_library(GLOG_LIBRARY NAMES glog  glogd libglog)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLog
  DEFAULT_MSG GLOG_LIBRARY GLOG_INCLUDE_DIR 
)
if(GLOG_FOUND AND NOT TARGET GLog::GLog)
    add_library(GLog::GLog UNKNOWN IMPORTED)
    set_target_properties(GLog::GLog PROPERTIES
        IMPORTED_LOCATION "${GLOG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLOG_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GLOG_INCLUDE_DIR GLOG_LIBRARY)
