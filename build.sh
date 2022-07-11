#!/bin/bash
set -e
set -x

CONFIG_AND_BUILD=true
# CONFIG_AND_BUILD=false

INSTALL_DIR="$PWD/llvm_install"

while $CONFIG_AND_BUILD ; do
  rm -rf _build
  mkdir _build
  break
done

cd _build

while $CONFIG_AND_BUILD; do

  #cmake --log-level=VERBOSE --debug-output \
  cmake \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DCMAKE_BUILD_TYPE="Debug" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    \
       ../llvm -G Ninja \
    \
    -DLLVM_TARGETS_TO_BUILD="LoongArch" \
    -DLLVM_ENABLE_PROJECTS="clang" \
    \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    \
    -DLLVM_INCLUDE_TOOLS=ON \
    -DLLVM_BUILD_TOOLS=ON \
    \
    -DLLVM_INCLUDE_UTILS=ON \
    -DLLVM_BUILD_UTILS=ON \
    \
    -DLLVM_INCLUDE_RUNTIMES=ON \
    -DLLVM_BUILD_RUNTIME=ON \
    \
    -DLLVM_INCLUDE_EXAMPLES=ON \
    -DLLVM_BUILD_EXAMPLES=OFF \
    \
    -DLLVM_INCLUDE_TESTS=ON \
    -DLLVM_BUILD_TESTS=OFF \
    \
    -DLLVM_INCLUDE_DOCS=ON \
    -DLLVM_BUILD_DOCS=OFF \
    -DLLVM_ENABLE_DOXYGEN=OFF \
    -DLLVM_ENABLE_SPHINX=OFF \
    -DLLVM_ENABLE_OCAMLDOC=OFF \
    -DLLVM_ENABLE_BINDINGS=OFF \
    \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    \
    -DLLVM_ENABLE_LIBCXX=OFF \
    -DLLVM_ENABLE_ZLIB=ON \
    -DLLVM_ENABLE_FFI=ON \
    -DLLVM_ENABLE_RTTI=ON \
    -DDEFAULT_SYSROOT="/data/lmx/loongson/loongarch64-linux-gnu-2021-08-09-vector/loongarch64-linux-gnu/sysroot" \
    -DLLVM_DEFAULT_TARGET_TRIPLE="loongarch64-unknown-linux-gnu"

  break
done

ninja  -j64

# ninja -v install
