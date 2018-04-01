#!/bin/bash

set -e

version=$(node -p 'require("./package.json").version')

sed -i'~' 's/set(\(rime_version\) .*)/set(\1 '$version')/' CMakeLists.txt
rm 'CMakeLists.txt~'
git add CMakeLists.txt

conventional-changelog -p angular -i CHANGELOG.md -s
git add CHANGELOG.md
