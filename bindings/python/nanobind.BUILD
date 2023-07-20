licenses(["notice"])

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "symboltable",
    srcs = ["cmake/darwin-ld-cpython.sym"],
)

cc_library(
    name = "nanobind",
    srcs = glob([
        "src/*.cpp"
    ]),
    copts = ["-fexceptions"],
    includes = ["include", "ext/robin_map/include"],
    textual_hdrs = glob(
        [
            "include/**/*.h",
            "src/*.h",
            "ext/robin_map/include/tsl/*.h",
        ],
    ),
    linkopts = select({
        "@platforms//os:macos": ["-Wl,@$(location :cmake/darwin-ld-cpython.sym)"],
        "//conditions:default": [],
    }),
    additional_linker_inputs = select({
        "@platforms//os:macos": [":cmake/darwin-ld-cpython.sym"],
        "//conditions:default": [],
    }),
    deps = ["@python_headers"],
)
