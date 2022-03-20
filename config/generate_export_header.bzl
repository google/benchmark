#
# Originl file is located at:
# https://github.com/RobotLocomotion/drake/blob/bad032aeb09b13c7f8c87ed64b624c8d1e9adb30/tools/workspace/generate_export_header.bzl
#
# All components of Drake are licensed under the BSD 3-Clause License
# shown below. Where noted in the source code, some portions may
# be subject to other permissive, non-viral licenses.
#
# Copyright 2012-2016 Robot Locomotion Group @ CSAIL
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.  Redistributions
# in binary form must reproduce the above copyright notice, this list of
# conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  Neither the name of
# the Massachusetts Institute of Technology nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# -*- python -*-

# Defines the implementation actions to generate_export_header.
def _generate_export_header_impl(ctx):
    windows_constraint = ctx.attr._windows_constraint[platform_common.ConstraintValueInfo]
    output = ctx.outputs.out

    if ctx.target_platform_has_constraint(windows_constraint):
      export_attr = "__declspec(dllexport)"
      import_attr = "__declspec(dllimport)"
      no_export_attr = ""
      deprecated_attr = "__declspec(deprecated)"
    else:
      export_attr = "__attribute__((visibility(\"default\")))"
      import_attr = "__attribute__((visibility(\"default\")))"
      no_export_attr = "__attribute__((visibility(\"hidden\")))"
      deprecated_attr = "__attribute__((__deprecated__))"

    content = [
        "#ifndef %s_H" % ctx.attr.export_macro_name,
        "#define %s_H" % ctx.attr.export_macro_name,
        "",
        "#ifdef %s" % ctx.attr.static_define,
        "#  define %s" % ctx.attr.export_macro_name,
        "#  define %s" % ctx.attr.no_export_macro_name,
        "#else",
        "#  ifndef %s" % ctx.attr.export_macro_name,
        "#    ifdef %s" % ctx.attr.export_import_condition,
        "#      define %s %s" % (ctx.attr.export_macro_name, export_attr),
        "#    else",
        "#      define %s %s" % (ctx.attr.export_macro_name, import_attr),
        "#    endif",
        "#  endif",
        "#  ifndef %s" % ctx.attr.no_export_macro_name,
        "#    define %s %s" % (ctx.attr.no_export_macro_name, no_export_attr),
        "#  endif",
        "#endif",
        "",
        "#ifndef %s" % ctx.attr.deprecated_macro_name,
        "#  define %s %s" % (ctx.attr.deprecated_macro_name, deprecated_attr),
        "#endif",
        "",
        "#ifndef %s" % ctx.attr.export_deprecated_macro_name,
        "#  define %s %s %s" % (ctx.attr.export_deprecated_macro_name, ctx.attr.export_macro_name, ctx.attr.deprecated_macro_name),  # noqa
        "#endif",
        "",
        "#ifndef %s" % ctx.attr.no_export_deprecated_macro_name,
        "#  define %s %s %s" % (ctx.attr.no_export_deprecated_macro_name, ctx.attr.no_export_macro_name, ctx.attr.deprecated_macro_name),  # noqa
        "#endif",
        "",
        "#endif",
    ]

    ctx.actions.write(output = output, content = "\n".join(content) + "\n")

# Defines the rule to generate_export_header.
_generate_export_header_gen = rule(
    attrs = {
        "out": attr.output(mandatory = True),
        "export_import_condition": attr.string(),
        "export_macro_name": attr.string(),
        "deprecated_macro_name": attr.string(),
        "export_deprecated_macro_name": attr.string(),
        "no_export_macro_name": attr.string(),
        "no_export_deprecated_macro_name": attr.string(),
        "static_define": attr.string(),
        "_windows_constraint": attr.label(default = "@platforms//os:windows"),
    },
    output_to_genfiles = True,
    implementation = _generate_export_header_impl,
)

def generate_export_header(
        lib = None,
        name = None,
        out = None,
        export_import_condition = None,
        export_macro_name = None,
        deprecated_macro_name = None,
        export_deprecated_macro_name = None,
        no_export_macro_name = None,
        no_export_deprecated_macro_name = None,
        static_define = None,
        **kwargs):
    """
    Creates a rule to generate an export header for a named library.
    
    This is an incomplete implementation of CMake's generate_export_header. (In
    particular, it assumes a platform that uses
    __attribute__((visibility("default"))) to decorate exports.)

    By default, the rule will have a mangled name related to the library name,
    and will produce "<lib>_export.h".

    The CMake documentation of the generate_export_header macro is:
    https://cmake.org/cmake/help/latest/module/GenerateExportHeader.html

    """

    if name == None:
        name = "__%s_export_h" % lib
    if out == None:
        out = "%s_export.h" % lib
    if export_import_condition == None:
        # CMake does not uppercase the <lib>_EXPORTS define.
        export_import_condition = "%s_EXPORTS" % lib
    if export_macro_name == None:
        export_macro_name = "%s_EXPORT" % lib.upper()
    if deprecated_macro_name == None:
        deprecated_macro_name = "%s_DEPRECATED" % lib.upper()
    if export_deprecated_macro_name == None:
        export_deprecated_macro_name = "%s_DEPRECATED_EXPORT" % lib.upper()
    if no_export_macro_name == None:
        no_export_macro_name = "%s_NO_EXPORT" % lib.upper()
    if no_export_deprecated_macro_name == None:
        no_export_deprecated_macro_name = \
            "%s_DEPRECATED_NO_EXPORT" % lib.upper()
    if static_define == None:
        static_define = "%s_STATIC_DEFINE" % lib.upper()

    _generate_export_header_gen(
        name = name,
        out = out,
        export_import_condition = export_import_condition,
        export_macro_name = export_macro_name,
        deprecated_macro_name = deprecated_macro_name,
        export_deprecated_macro_name = export_deprecated_macro_name,
        no_export_macro_name = no_export_macro_name,
        no_export_deprecated_macro_name = no_export_deprecated_macro_name,
        static_define = static_define,
        **kwargs
    )
