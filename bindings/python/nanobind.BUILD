licenses(["notice"])

package(default_visibility = ["//visibility:public"])

config_setting(
    name = "msvc_compiler",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "msvc-cl"},
)

cc_library(
    name = "nanobind",
    srcs = glob([
        "src/*.cpp",
    ]),
    additional_linker_inputs = select({
        "@platforms//os:macos": [":cmake/darwin-ld-cpython.sym"],
        "//conditions:default": [],
    }),
    copts = select({
        ":msvc_compiler": [
            "/EHsc",  # exceptions
            "/Os",  # size optimizations
        ],
        # these should work on both clang and gcc.
        "//conditions:default": [
            "-fexceptions",
            "-flto",
            "-Os",
        ],
    }),
    includes = [
        "ext/robin_map/include",
        "include",
    ],
    linkopts = select({
        "@platforms//os:macos": ["-Wl,@$(location :cmake/darwin-ld-cpython.sym)"],
        "//conditions:default": [],
    }),
    textual_hdrs = glob(
        [
            "include/**/*.h",
            "src/*.h",
            "ext/robin_map/include/tsl/*.h",
        ],
    ),
    deps = ["@python_headers"],
)
