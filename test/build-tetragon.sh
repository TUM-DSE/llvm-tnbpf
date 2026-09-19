#!/bin/sh
set -x

# git clone --branch v1.7 git@github.com:cilium/tetragon.git

export LOCAL_CLANG=1
export CLANG="$(pwd)/../build-release/bin/clang"
export PATH="${PATH}:$(pwd)/../build-release/bin"

cd ./tetragon

make clean

sed -i 's/SHELL=\/bin\/bash/SHELL=\/usr\/bin\/env bash/g' bpf/Makefile.defs

make -j32 tetragon-bpf
