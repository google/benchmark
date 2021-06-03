#!/usr/bin/env bash

# Checkout LLVM sources
git clone --depth=1 https://github.com/llvm/llvm-project.git llvm-project

# Setup libc++ options
if [ -z "$BUILD_32_BITS" ]; then
  export BUILD_32_BITS=OFF && echo disabling 32 bit build
fi

# Build and install libc++ (Use unstable ABI for better sanitizer coverage)
cmake -DCMAKE_C_COMPILER=${C_COMPILER}          \
      -DCMAKE_CXX_COMPILER=${COMPILER}          \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo         \
      -DCMAKE_INSTALL_PREFIX=/usr               \
      -DLIBCXX_ABI_UNSTABLE=OFF                 \
      -DLLVM_USE_SANITIZER=${LIBCXX_SANITIZER}  \
      -DLLVM_BUILD_32_BITS=${BUILD_32_BITS}     \
      -DLLVM_ENABLE_PROJECTS='clang;libcxx;libcxxabi' \
      -S llvm -B llvm-build -G "Unix Makefiles"
make -j2
sudo make install-libcxxabi install-libcxx install-clang
