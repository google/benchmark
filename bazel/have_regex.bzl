def have_regex_copts():
  return select({
      "//bazel:have_std_regex": ["-DHAVE_STD_REGEX"],
      "//bazel:have_posix_regex": ["-DHAVE_POSIX_REGEX"],
      "//bazel:have_gnu_posix_regex": ["-DHAVE_GNU_POSIX_REGEX"],
      "//conditions:default": ["-DHAVE_STD_REGEX"],
  })
