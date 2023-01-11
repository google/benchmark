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
        "src/nb_type.cpp",
        "src/tensor.cpp",
        "src/trampoline.cpp",
    ],
    copts = [
        "-fexceptions",
        "-std=c++17", # requires C++17
        "-Os",  # size optimization option for nanobind
        "-flto", # enable LTO
    ],
    linkopts = ["-undefined suppress", "-flat_namespace", "-flto"],
    includes = ["include", "ext/robin_map/include"],
    deps = ["@python_headers"],
    visibility = ["//visibility:public"],
)
