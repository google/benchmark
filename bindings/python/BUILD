load("@rules_python//python:packaging.bzl", "py_package", "py_wheel")

exports_files(glob(["*.BUILD"]))
exports_files(["build_defs.bzl"])



py_package(
    name = "pkg",
    # Only include these Python packages.
    packages = ["examples.wheel"],
    deps = [":main"],
)

py_wheel(
    name = "google-benchmark",
    version="", # TODO
    url="https://github.com/google/benchmark",
    description="A library to benchmark code snippets.",
    long_description="",,
    long_description_content_type="text/markdown",
    author="Google",
    author_email="benchmark-py@google.com",
    install_requires=_parse_requirements("bindings/python/requirements.txt"),
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
    platform = select({
        ":linux_x86_64_local": "linux_x86_64",
        ":linux_x86_64_release": "manylinux2014_x86_64",
        ":linux_aarch64_local": "linux_aarch64",
        ":linux_aarch64_release": "manylinux2014_aarch64",
        ":osx_universal2": "macosx_10_9_universal2",
        ":osx_aarch64": "macosx_11_0_arm64",
        ":windows_x86_32": "win32",
        ":windows_x86_64": "win_amd64",
        "//conditions:default": "any",
    }),
    deps = [
        ":pkg",
    ],
)
