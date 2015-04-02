#!/usr/bin/env bash

set -x
set -e

# Before install
if [ "$STD" = "c++11" ]; then
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    if [ "$CXX" = "clang++" ]; then
        deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.6 main
        deb-src http://llvm.org/apt/precise/ llvm-toolchain-precise-3.6 main
        wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -
    fi
    sudo apt-get update -qq
fi

# Install
if [ "$STD" = "c++11" && "$CXX" = "g++" ]; then
    sudo apt-get install -qq gcc-4.8 g++-4.8
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 90
fi
elif [ "$CXX" = "clang++" ]; then
    sudo apt-get install clang-3.6
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.6 90
fi
# Install cmake
wget http://www.cmake.org/files/v3.2/cmake-3.2.1-Linux-x86_64.tar.gz
sudo tar -C /opt -xzvf cmake-3.2.1-Linux-x86_64.tar.gz
rm cmake-3.2.1-Linux-x86_64.tar.gz
export PATH=/opt/cmake-3.2.1-Linux-x86_64/bin:$PATH
