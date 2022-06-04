#!/bin/bash

set -e

cd "$(dirname "$0")"

clone_options=(
    # for GitHub pull request #1, git checkout 1/merge
    --config 'remote.origin.fetch=+refs/pull/*:refs/remotes/origin/*'
    # shallow clone
    --depth 1
    # fetch all branches
    --no-single-branch
)

if [[ "${1}" =~ run=.* ]]; then
    custom_install="${1#run=}"
    shift
fi

for plugin in "$@"
do
    if [[ "${plugin}" =~ @ ]]; then
        slug="${plugin%@*}"
        branch="${plugin#*@}"
    else
        slug="${plugin}"
        branch=''
    fi
    plugin_project="${slug##*/}"
    plugin_dir="plugins/${plugin_project#librime-}"
    if [[ -d "${plugin_dir}" ]]
    then
        echo "Updating ${plugin} in ${plugin_dir}"
        if [[ -n "${branch}" ]]; then
            git -C "${plugin_dir}" checkout "${branch}"
        fi
        git -C "${plugin_dir}" pull
    else
        echo "Checking out ${plugin} to ${plugin_dir}"
        git clone "${clone_options[@]}" "https://github.com/${slug}.git" "${plugin_dir}"
        # pull request ref doesn't work with git clone --branch
        if [[ -n "${branch}" ]]; then
            git -C "${plugin_dir}" checkout "${branch}"
        fi
    fi
    if [[ -n "${custom_install}" ]]; then
        ${custom_install} "${plugin}" "${plugin_dir}"
    fi
done
