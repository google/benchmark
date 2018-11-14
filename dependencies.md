# Build tool dependency policy

To ensure the broadest compatibility when building the benchmark library, but
still allow forward progress, we require any build tooling to be available for:

* Debian stable AND
* The last two Ubuntu LTS releases

Currently, this means the highest build tool versions we can use must be
available for Ubuntu 16.04 (Xenial), Ubuntu 18.04 (Bionic), and Debian stretch.

However, there are projects that rely on Benchmark that require a slightly older
version than this
([LVM](https://github.com/llvm-mirror/llvm/blob/master/CMakeLists.txt)], so we
will choose something that allows this compatibility requirement to be met.

_Note, [travis](.travis.yml) runs under Ubuntu 14.04 (Trusty) for linux builds._

## cmake
The current supported version is cmake 3.4.3 as of 2018-11-14.

_Note, this version is also available for Ubuntu 14.04, the previous Ubuntu LTS
release, as `cmake3`._
