
config_setting(
    name = "msvc_compiler",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "msvc-cl"},
)

cc_library(
    name = "nanobind",
    hdrs = glob(
        include = [
            "include/nanobind/*.h",
            "include/nanobind/stl/*.h",
            "include/nanobind/detail/*.h",
        ],
        exclude = [],
    ),
    srcs = [
        "include/nanobind/stl/detail/nb_dict.h",
        "include/nanobind/stl/detail/nb_list.h",
        "include/nanobind/stl/detail/traits.h",
        "ext/robin_map/include/tsl/robin_map.h",
        "ext/robin_map/include/tsl/robin_hash.h",
        "ext/robin_map/include/tsl/robin_growth_policy.h",
        "ext/robin_map/include/tsl/robin_set.h",
        "src/buffer.h",
        "src/common.cpp",
        "src/error.cpp",
        "src/implicit.cpp",
        "src/nb_enum.cpp",
        "src/nb_func.cpp",
        "src/nb_internals.cpp",
        "src/nb_internals.h",
        "src/nb_ndarray.cpp",
        "src/nb_type.cpp",
        "src/trampoline.cpp",
    ],
    copts = select({
        ":msvc_compiler": [],
        "//conditions:default": [
        "-fexceptions",
        "-Os",  # size optimization
        "-flto", # enable LTO
        ],
    }),
    linkopts = select({
        "@com_github_google_benchmark//:macos": [
        "-undefined dynamic_lookup",
        "-Wl,-no_fixup_chains",
        "-Wl,-dead_strip",
        ],
        "//conditions:default": [],
    }),
    includes = ["include", "ext/robin_map/include"],
    deps = ["@python_headers"],
    visibility = ["//visibility:public"],
)
