#!/usr/bin/env bash

# Before install
if [ "$STD" = "c++11" ]; then
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    if [ "$CXX" = "clang++" ]; then
        wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -
        sudo add-apt-repository -y "deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.6 main"
    fi
    sudo apt-get update -qq
fi

# Install
if [ "$STD" = "c++11" ] && [ "$CXX" = "g++" ]; then
    sudo apt-get install -qq gcc-4.8 g++-4.8
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 90
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 90
elif [ "$CXX" = "clang++" ]; then
    sudo apt-get install -qq clang-3.6
    sudo update-alternatives --install /usr/local/bin/clang   clang   /usr/bin/clang-3.6 90
    sudo update-alternatives --install /usr/local/bin/clang++ clang++ /usr/bin/clang++-3.6 90
    export PATH=/usr/local/bin:$PATH
fi
# Install cmake
wget http://www.cmake.org/files/v3.2/cmake-3.2.1-Linux-x86_64.tar.gz
sudo tar -C /opt -xzf cmake-3.2.1-Linux-x86_64.tar.gz
rm cmake-3.2.1-Linux-x86_64.tar.gz
export PATH=/opt/cmake-3.2.1-Linux-x86_64/bin:$PATH
