#!/bin/bash

export RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

echo "RIME_PLUGINS=${RIME_PLUGINS}" > version-info.txt
echo "librime $(git describe --always)" >> version-info.txt

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh ${RIME_PLUGINS}
    for plugin_dir in plugins/*; do
        [[ -d "${plugin_dir}" ]] || continue
        echo "${plugin_dir} $(git -C "${plugin_dir}" describe --always)" >> version-info.txt
        if [[ -e "${plugin_dir}/action-install.sh" ]]; then
	        (cd "${plugin_dir}"; bash ./action-install.sh)
        fi
    done
fi
