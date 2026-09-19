#!/bin/sh
set -x
export CLANG="$(pwd)/../../build-debug/bin/clang"
export PATH="${PATH}:$(pwd)/../../build-release/bin"

cd ./cilium/bpf

make -j32
