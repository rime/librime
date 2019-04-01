#!/bin/bash

cd "$(dirname "$0")"

for slug in "$@"
do
    plugin_project="${slug##*/}"
    plugin_dir="plugins/${plugin_project#librime-}"
    if [[ -d "${plugin_dir}" ]]
    then
        echo "Updating plugin: ${plugin_dir}"
        git -C "${plugin_dir}" checkout master
        git -C "${plugin_dir}" pull
    else
        git clone --depth 1 "https://github.com/${slug}.git" "${plugin_dir}"
    fi
done
