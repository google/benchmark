import contextlib
import os
import platform
import re
import shutil
import sys
import sysconfig
from collections.abc import Generator
from pathlib import Path
from typing import Any

import setuptools
from setuptools.command import bdist_wheel, build_ext

IS_WINDOWS = platform.system() == "Windows"
IS_MAC = platform.system() == "Darwin"
IS_LINUX = platform.system() == "Linux"
IS_FREE_THREADED = bool(sysconfig.get_config_var("Py_GIL_DISABLED"))

# Build in nanobind 3 split mode targeting stable ABI for:
# - Standard CPython 3.10+ (cp310-abi3)
# - Free-threaded CPython 3.15t+ (cp315-abi3t, PEP 803)
# Free-threaded CPython 3.14t has no stable ABI and is built in linked mode.
SPLIT_MODE = not IS_FREE_THREADED or sys.version_info >= (3, 15)
SABI_TARGET = "cp315t" if IS_FREE_THREADED else "cp310"
if IS_WINDOWS:
    EXT_SUFFIX = ".pyd"
elif SPLIT_MODE:
    EXT_SUFFIX = ".abi3t.so" if IS_FREE_THREADED else ".abi3.so"
else:
    EXT_SUFFIX = ".so"
py_limited_api = SPLIT_MODE
options = (
    {"bdist_wheel": {"py_limited_api": "cp310"}} if not IS_FREE_THREADED else {}
)


def is_cibuildwheel() -> bool:
    return os.getenv("CIBUILDWHEEL") is not None


@contextlib.contextmanager
def _maybe_patch_toolchains() -> Generator[None, None, None]:
    """
    Patch rules_python toolchains to ignore root user error
    when run in a Docker container on Linux in cibuildwheel.
    """

    def fmt_toolchain_args(matchobj):
        suffix = "ignore_root_user_error = True"
        callargs = matchobj.group(1)
        # toolchain def is broken over multiple lines
        if callargs.endswith("\n"):
            callargs = callargs + "    " + suffix + ",\n"
        # toolchain def is on one line.
        else:
            callargs = callargs + ", " + suffix
        return "python.toolchain(" + callargs + ")"

    CIBW_LINUX = is_cibuildwheel() and IS_LINUX
    module_bazel = Path("MODULE.bazel")
    content: str = module_bazel.read_text()
    try:
        if CIBW_LINUX:
            module_bazel.write_text(
                re.sub(
                    r"python.toolchain\(([\w\"\s,.=]*)\)",
                    fmt_toolchain_args,
                    content,
                )
            )
        yield
    finally:
        if CIBW_LINUX:
            module_bazel.write_text(content)


class BazelExtension(setuptools.Extension):
    """A C/C++ extension that is defined as a Bazel BUILD target."""

    def __init__(self, name: str, bazel_target: str, **kwargs: Any):
        super().__init__(name=name, sources=[], **kwargs)

        self.bazel_target = bazel_target
        stripped_target = bazel_target.split("//")[-1]
        self.relpath, self.target_name = stripped_target.split(":")


class BdistWheel(bdist_wheel.bdist_wheel):
    """Custom bdist_wheel command to handle PEP 803 abi3t wheel tags."""

    def get_tag(self) -> tuple[str, str, str]:
        python, abi, plat = super().get_tag()
        if IS_FREE_THREADED and sys.version_info >= (3, 15):
            return "cp315", "abi3t", plat
        return python, abi, plat


class BuildBazelExtension(build_ext.build_ext):
    """A command that runs Bazel to build a C/C++ extension."""

    def run(self):
        for ext in self.extensions:
            self.bazel_build(ext)
        # explicitly call `bazel shutdown` for graceful exit
        self.spawn(["bazel", "shutdown"])

    def copy_extensions_to_source(self):
        """
        Copy generated extensions into the source tree.
        This is done in the ``bazel_build`` method, so it's not necessary to
        do again in the `build_ext` base class.
        """

    def bazel_build(self, ext: BazelExtension) -> None:
        """Runs the bazel build to create the package."""
        temp_path = Path(self.build_temp)

        # We round to the minor version, which makes rules_python
        # look up the latest available patch version internally.
        python_version = "{}.{}".format(*sys.version_info[:2])

        bazel_argv = [
            "bazel",
            "run",
            ext.bazel_target,
            f"--symlink_prefix={temp_path / 'bazel-'}",
            f"--compilation_mode={'dbg' if self.debug else 'opt'}",
            # C++17 is required by nanobind
            f"--cxxopt={'/std:c++17' if IS_WINDOWS else '-std=c++17'}",
            f"--@rules_python//python/config_settings:python_version={python_version}",
        ]

        if IS_FREE_THREADED:
            bazel_argv.append(
                "--@rules_python//python/config_settings:py_freethreaded=yes"
            )

        if SPLIT_MODE:
            bazel_argv += [
                f"--@nanobind_bazel//:py-limited-api={SABI_TARGET}",
                "--@nanobind_bazel//:split-mode=True",
            ]

        if IS_WINDOWS:
            # Link with python*.lib.
            for library_dir in self.library_dirs:
                bazel_argv.append("--linkopt=/LIBPATH:" + library_dir)
        elif IS_MAC:
            # C++17 needs macOS 10.14 at minimum
            bazel_argv.append("--macos_minimum_os=10.14")

        with _maybe_patch_toolchains():
            self.spawn(bazel_argv)

        # copy the Bazel build artifacts into setuptools' libdir,
        # from where the wheel is built.
        pkgname = "google_benchmark"
        pythonroot = Path("bindings") / "python" / "google_benchmark"
        srcdir = temp_path / "bazel-bin" / pythonroot
        if not self.inplace:
            libdir = Path(self.build_lib) / pkgname
        else:
            build_py = self.get_finalized_command("build_py")
            libdir = Path(build_py.get_package_dir(pkgname))

        for root, dirs, files in os.walk(srcdir, topdown=True):
            # exclude runfiles directories and children.
            dirs[:] = [d for d in dirs if "runfiles" not in d]

            for f in files:
                fp = Path(f)
                # we do not want the bare .so file included
                # when building for ABI3, so we require a
                # full and exact match on the file extension.
                if "".join(fp.suffixes) == EXT_SUFFIX or fp.suffix == ".pyi":
                    shutil.copyfile(root / fp, libdir / fp)


setuptools.setup(
    cmdclass={"build_ext": BuildBazelExtension, "bdist_wheel": BdistWheel},
    install_requires=["nanobind-backend>=1.0"] if SPLIT_MODE else [],
    package_data={"google_benchmark": ["py.typed", "*.pyi"]},
    ext_modules=[
        BazelExtension(
            name="google_benchmark._benchmark",
            bazel_target="//bindings/python/google_benchmark:benchmark_stubgen",
            py_limited_api=py_limited_api,
        )
    ],
    options=options,
)
