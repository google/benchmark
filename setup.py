import contextlib
import os
import platform
import shutil
import sysconfig
from pathlib import Path
from typing import List

import setuptools
from setuptools.command import build_ext


PYTHON_INCLUDE_PATH_PLACEHOLDER = "<PYTHON_INCLUDE_PATH>"

IS_WINDOWS = platform.system() == "Windows"
IS_MAC = platform.system() == "Darwin"


def _get_long_description(fp: str) -> str:
    with open(fp, "r", encoding="utf-8") as f:
        return f.read()


def _get_version(fp: str) -> str:
    """Parse a version string from a file."""
    with open(fp, "r") as f:
        for line in f:
            if "__version__" in line:
                delim = '"'
                return line.split(delim)[1]
    raise RuntimeError(f"could not find a version string in file {fp!r}.")


def _parse_requirements(fp: str) -> List[str]:
    with open(fp) as requirements:
        return [
            line.rstrip()
            for line in requirements
            if not (line.isspace() or line.startswith("#"))
        ]


@contextlib.contextmanager
def temp_fill_include_path(fp: str):
    """Temporarily set the Python include path in a file."""
    with open(fp, "r+") as f:
        try:
            content = f.read()
            replaced = content.replace(
                PYTHON_INCLUDE_PATH_PLACEHOLDER,
                Path(sysconfig.get_paths()['include']).as_posix(),
            )
            f.seek(0)
            f.write(replaced)
            f.truncate()
            yield
        finally:
            # revert to the original content after exit
            f.seek(0)
            f.write(content)
            f.truncate()


class BazelExtension(setuptools.Extension):
    """A C/C++ extension that is defined as a Bazel BUILD target."""

    def __init__(self, name: str, bazel_target: str):
        super().__init__(name=name, sources=[])

        self.bazel_target = bazel_target
        stripped_target = bazel_target.split("//")[-1]
        self.relpath, self.target_name = stripped_target.split(":")


class BuildBazelExtension(build_ext.build_ext):
    """A command that runs Bazel to build a C/C++ extension."""

    def run(self):
        for ext in self.extensions:
            self.bazel_build(ext)
        build_ext.build_ext.run(self)

    def bazel_build(self, ext: BazelExtension):
        """Runs the bazel build to create the package."""
        with temp_fill_include_path("WORKSPACE"):
            temp_path = Path(self.build_temp)

            bazel_argv = [
                "bazel",
                "build",
                ext.bazel_target,
                f"--symlink_prefix={temp_path / 'bazel-'}",
                f"--compilation_mode={'dbg' if self.debug else 'opt'}",
                # C++17 is required by nanobind
                f"--cxxopt={'/std:c++17' if IS_WINDOWS else '-std=c++17'}",
            ]

            if IS_WINDOWS:
                # Link with python*.lib.
                for library_dir in self.library_dirs:
                    bazel_argv.append("--linkopt=/LIBPATH:" + library_dir)
            elif IS_MAC:
                if platform.machine() == "x86_64":
                    # C++17 needs macOS 10.14 at minimum
                    bazel_argv.append("--macos_minimum_os=10.14")

                    # cross-compilation for Mac ARM64 on GitHub Mac x86 runners.
                    # ARCHFLAGS is set by cibuildwheel before macOS wheel builds.
                    archflags = os.getenv("ARCHFLAGS", "")
                    if "arm64" in archflags:
                        bazel_argv.append("--cpu=darwin_arm64")
                        bazel_argv.append("--macos_cpus=arm64")

                elif platform.machine() == "arm64":
                    bazel_argv.append("--macos_minimum_os=11.0")

            self.spawn(bazel_argv)

            shared_lib_suffix = '.dll' if IS_WINDOWS else '.so'
            ext_name = ext.target_name + shared_lib_suffix
            ext_bazel_bin_path = temp_path / 'bazel-bin' / ext.relpath / ext_name

            ext_dest_path = Path(self.get_ext_fullpath(ext.name))
            shutil.copyfile(ext_bazel_bin_path, ext_dest_path)

            # explicitly call `bazel shutdown` for graceful exit
            self.spawn(["bazel", "shutdown"])


setuptools.setup(
    name="google_benchmark",
    version=_get_version("bindings/python/google_benchmark/__init__.py"),
    url="https://github.com/google/benchmark",
    description="A library to benchmark code snippets.",
    long_description=_get_long_description("README.md"),
    long_description_content_type="text/markdown",
    author="Google",
    author_email="benchmark-py@google.com",
    # Contained modules and scripts.
    package_dir={"": "bindings/python"},
    packages=setuptools.find_packages("bindings/python"),
    install_requires=_parse_requirements("bindings/python/requirements.txt"),
    cmdclass=dict(build_ext=BuildBazelExtension),
    ext_modules=[
        BazelExtension(
            "google_benchmark._benchmark",
            "//bindings/python/google_benchmark:_benchmark",
        )
    ],
    zip_safe=False,
    # PyPI package information.
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Software Development :: Testing",
        "Topic :: System :: Benchmark",
    ],
    license="Apache 2.0",
    keywords="benchmark",
)
