#!/usr/bin/env bash

curl -sSL https://cmake.org/files/v3.6/cmake-3.6.1-Linux-x86_64.sh -o install-cmake.sh
chmod +x install-cmake.sh
sudo ./install-cmake.sh --prefix=/usr/local --skip-license
git clone --depth=1 https://github.com/llvm-mirror/llvm.git llvm-source
cd llvm-source/projects
git clone --depth=1 https://github.com/llvm-mirror/libcxx.git
git clone --depth=1 https://github.com/llvm-mirror/libcxxabi.git
cd ../.. && mkdir llvm-build && cd llvm-build
cmake -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${COMPILER} -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER=${LLVM_SAN} ../llvm-source
make cxx
sudo make install-libcxxabi install-libcxx
cd ../

