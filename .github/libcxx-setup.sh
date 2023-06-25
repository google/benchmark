#!/usr/bin/env bash

set -e

Checkout LLVM sources
git clone --depth=1 https://github.com/llvm/llvm-project.git llvm-project

Set default values for optional variables
: "${BUILD_32_BITS:=OFF}" # If BUILD_32_BITS is not set, default to OFF

Build and install libc++
mkdir -p llvm-build
cd llvm-build
cmake_args=(
"-DCMAKE_C_COMPILER=${CC}"
"-DCMAKE_CXX_COMPILER=${CXX}"
"-DCMAKE_BUILD_TYPE=RelWithDebInfo"
"-DCMAKE_INSTALL_PREFIX=/usr"
"-DLIBCXX_ABI_UNSTABLE=OFF"
"-DLLVM_USE_SANITIZER=${LIBCXX_SANITIZER}"
"-DLLVM_BUILD_32_BITS=${BUILD_32_BITS}"
"-DLLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind"
"-G" "Unix Makefiles"
"../llvm-project/runtimes/"
)
cmake "${cmake_args[@]}"
make -j$(nproc) cxx cxxabi unwind
cd ..
