find_path(YAML_CPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h)

# Find library
find_library(YAML_CPP_LIBRARY NAMES yaml-cpp libyaml-cpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YamlCpp
    DEFAULT_MSG YAML_CPP_LIBRARY YAML_CPP_INCLUDE_DIR
)

if(YAMLCPP_FOUND AND NOT TARGET YamlCpp::YamlCpp)
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY})
    add_library(YamlCpp::YamlCpp UNKNOWN IMPORTED)
    set_target_properties(YamlCpp::YamlCpp PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${YAML_CPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY)
