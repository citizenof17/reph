#! /bin/sh

set -ex

mkdir build || true
cmake CMakeLists.txt -B build
cd build || exit
make

