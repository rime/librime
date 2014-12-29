# - Locate the rime library
# This module defines
#  Rime_FOUND found librime
#  Rime_LIBRARIES rime library path
#  Rime_INCLUDE_DIR, where to find headers

#==============================================================================
# Copyright 2012 Xuetian Weng
#
# Distributed under the BSD License
#==============================================================================

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

if(Rime_INCLUDE_DIR AND Rime_LIBRARIES)
    # Already in cache, be silent
    set(Rime_FIND_QUIETLY TRUE)
endif(Rime_INCLUDE_DIR AND Rime_LIBRARIES)

include(FindPkgConfig)
PKG_CHECK_MODULES(PC_Rime rime)

find_path(Rime_INCLUDE_DIR
          NAMES rime_api.h
          HINTS ${PC_Rime_INCLUDEDIR}
          )

find_library(Rime_LIBRARIES
             NAMES rime
             HINTS ${PC_Rime_LIBDIR}
             )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Rime DEFAULT_MSG Rime_LIBRARIES Rime_INCLUDE_DIR)
