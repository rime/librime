#!/bin/bash

set -e

new_version="$(git-cliff --bumped-version)"

sed -i'~' 's/set(\(rime_version\) .*)/set(\1 '${new_version}')/' CMakeLists.txt
rm 'CMakeLists.txt~'
git add CMakeLists.txt

git-cliff --bump --unreleased --prepend CHANGELOG.md
${VISUAL:-${EDITOR:-nano}} CHANGELOG.md
git add CHANGELOG.md

release_message="chore(release): ${new_version} :tada:"
release_tag="${new_version}"
git commit --all --message "${release_message}"
git tag --annotate "${release_tag}" --message "${release_message}"
