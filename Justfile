help:
    just -l

clean:
    rm -rf build

configure:
    #!/usr/bin/env bash
    mkdir -p build
    cd build
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

build:
    ninja -Cbuild


compile-bpf:
    ./build/bin/clang -O2 -g -I"${LINUX_HEADERS}/include" -I"${LIBBPF}/include" -target bpf -c test/hello.bpf.c -o test/hello.bpf.o

