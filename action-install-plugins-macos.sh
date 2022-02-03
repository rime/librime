#!/bin/bash

export RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh ${RIME_PLUGINS}
    for plugin_dir in plugins/*; do
        if [[ -e "${plugin_dir}/action-install.sh" ]]; then
	        (cd "${plugin_dir}"; bash ./action-install.sh)
        fi
    done
fi
