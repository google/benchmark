def _generate_version_header_impl(ctx):

    args = ["--header", ctx.outputs.header.path] + ["--header_input", ctx.file.src.path]\
            + ["--volatile_file", ctx.version_file.path, \
            "--version_variable_name", ctx.attr.git_version_name, "--is_dirty_name",\
            ctx.attr.git_is_dirty_name, "--default_version", ctx.attr.default_version]

    ctx.actions.run(
        inputs = [ctx.version_file, ctx.info_file, ctx.file.src],
        outputs = [ctx.outputs.header],
        arguments = args,
        executable = ctx.executable._get_git_version_tool,
    )

generate_version_header = rule(
    implementation = _generate_version_header_impl,
    attrs = {
        "_get_git_version_tool": attr.label(
            executable = True,
            cfg = "host",
            allow_files = True,
            default = Label("//:get_git_version"),
        ),
        "git_version_name": attr.string(mandatory = True),
        "git_is_dirty_name": attr.string(mandatory = True),
        "default_version": attr.string(mandatory = True),
        "header": attr.output(mandatory = True),
        "src" : attr.label(allow_single_file = [".in"]),
    },
)
