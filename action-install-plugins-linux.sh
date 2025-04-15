#!/bin/bash

export RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

echo "RIME_PLUGINS=${RIME_PLUGINS}" > version-info.txt
echo "librime $(git describe --always)" >> version-info.txt

function action_install_plugin() {
    local plugin="$1"
    local plugin_dir="$2"
    echo "${plugin} $(git -C "${plugin_dir}" describe --always)" >> version-info.txt
    if [[ -e "${plugin_dir}/action-install.sh" ]]; then
	    (cd "${plugin_dir}"; bash ./action-install.sh)
    fi
}

declare -fx action_install_plugin

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh run=action_install_plugin ${RIME_PLUGINS}
fi
