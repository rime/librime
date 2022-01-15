#!/bin/bash

# install and build boost
make xcode/thirdparty/boost

export BOOST_ROOT="$(pwd)/thirdparty/src/boost_1_75_0"

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
