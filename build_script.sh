#! /bin/sh

set -ex

rmdir -rf build || true
mkdir build || true
cmake CMakeLists.txt -B build
cd build || exit
make

