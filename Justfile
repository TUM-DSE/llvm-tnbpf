help:
    just -l

clean:
    rm -rf build-debug
    rm -rf build-release

configure:
    #!/usr/bin/env bash
    mkdir -p build-debug
    cd build-debug
    cmake -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" \
        -DLLVM_TARGETS_TO_BUILD="BPF" \
        -DCMAKE_BUILD_TYPE="Debug" \
        -DLLVM_ENABLE_ASSERTIONS="ON" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" \
        -DLLVM_CCACHE_BUILD="ON" \
        -DLLVM_ENABLE_LLD="OFF" \
        -DLLVM_USE_LINKER="mold" \
        -DCMAKE_C_COMPILER="clang" \
        -DCMAKE_CXX_COMPILER="clang++" \
        -DLLVM_PARALLEL_LINK_JOBS=1 \
        -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
        -G "Ninja" \
        ../llvm
    cd ..
    mkdir -p build-release
    cd build-release
    cmake -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" \
       -DLLVM_TARGETS_TO_BUILD="BPF" \
       -DCMAKE_BUILD_TYPE="Release" \
       -DLLVM_ENABLE_ASSERTIONS="ON" \
       -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" \
       -DLLVM_CCACHE_BUILD="ON" \
       -DLLVM_ENABLE_LLD="OFF" \
       -DLLVM_USE_LINKER="mold" \
       -DCMAKE_C_COMPILER="clang" \
       -DCMAKE_CXX_COMPILER="clang++" \
       -DLLVM_PARALLEL_LINK_JOBS=1 \
       -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
       -G "Ninja" \
       ../llvm

build-debug:
    ninja -Cbuild-debug


build-release:
    ninja -Cbuild-release

compile-bpf-debug:
    ./build-debug/bin/clang -mllvm -debug -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c test/hello.bpf.c -o test/hello.bpf.o

compile-bpf-ir-debug:
   ./build-debug/bin/clang -S -emit-llvm -mllvm -debug -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c test/hello.bpf.c -o test/hello.bpf.ll


compile-bpf-release:
    ./build-release/bin/clang -mllvm -debug -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c test/hello.bpf.c -o test/hello.bpf.o

compile-bpf-ir-release:
   ./build-release/bin/clang -S -emit-llvm -mllvm -debug -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c test/hello.bpf.c -o test/hello.bpf.ll

