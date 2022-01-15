#!/bin/bash

export boost_version="${boost_version=1.75.0}"
boost_x_y_z="${boost_version//./_}"

export BOOST_ROOT="${BOOST_ROOT=$(pwd)/thirdparty/src/boost_${boost_x_y_z}}"

# TODO: cache dependencies

# install and build boost
make xcode/thirdparty/boost

make xcode/thirdparty

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh ${RIME_PLUGINS}
    for plugin_dir in plugins/*; do
        if [[ -e "${plugin_dir}/action-install.sh" ]]; then
	        (cd "${plugin_dir}"; bash ./action-install.sh)
        fi
    done
fi

make xcode
