def _generate_version_header_impl(ctx):
   output = ctx.outputs.out
   content = [
        "#ifndef VERSION_CONFIG_H",
        "#define VERSION_CONFIG_H",
        "// clang-format off",
        "#define LIBBENCHMARK_MAJOR_VERSION %d" % ctx.attr.major,
        "#define LIBBENCHMARK_MINOR_VERSION %d" % ctx.attr.minor,
        "#define LIBBENCHMARK_PATCH_VERSION %d" % ctx.attr.patch,
        "// clang-format on",
        "#endif  // VERSION_CONFIG_H",
    ]

   ctx.actions.write(output = output, content = "\n".join(content) + "\n")

generate_version_header = rule(
    implementation = _generate_version_header_impl,
    attrs = {
        "out": attr.output(mandatory = True),
        "major": attr.int(mandatory = True),
        "minor": attr.int(mandatory = True),
        "patch": attr.int(mandatory = True),
    },
)
