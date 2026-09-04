#!/bin/bash
set -e

# https://apt.llvm.org/
sudo apt install -y wget
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh

sudo ./llvm.sh "$1"
sudo apt install -y clang-format-"$1"

sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-"$1" 100
sudo update-alternatives --set clang-format /usr/bin/clang-format-"$1"
