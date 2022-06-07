def _generate_version_header_impl(ctx):
   output = ctx.outputs.out
   content = [
        "#ifndef VERSION_CONFIG_H",
        "#define VERSION_CONFIG_H",
        "// clang-format off",
        "#define LIBBENCHMARK_VERSION \"%s\"" % ctx.attr.git_version,
        "// clang-format on",
        "#endif  // VERSION_CONFIG_H",
    ]

   ctx.actions.write(output = output, content = "\n".join(content) + "\n")

generate_version_header = rule(
    implementation = _generate_version_header_impl,
    attrs = {
        "out": attr.output(mandatory = True),
        "git_version": attr.string(mandatory = True),
    },
)
