#!/bin/bash

for slug in "$@"
do
    plugin_project="${slug##*/}"
    plugin_dir="plugins/${plugin_project#librime-}"
    git clone --depth 1 "https://github.com/${slug}.git" "${plugin_dir}"

    if [[ -e "${plugin_dir}/travis-install.sh" ]]; then
	bash "${plugin_dir}/travis-install.sh"
    fi
done
