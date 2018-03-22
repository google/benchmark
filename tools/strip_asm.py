#!/usr/bin/env python

"""
strip_asm.py - Cleanup ASM output for the specified file
"""

from argparse import ArgumentParser
import sys
import os
import re

def find_used_labels(asm):
    found = set()
    label_re = re.compile("\s*j[a-z]+\s+\.L([a-zA-Z0-9][a-zA-Z0-9_]*)")
    for l in asm.splitlines():
        m = label_re.match(l)
        if m:
            found.add('.L%s' % m.group(1))
    return found

def process_asm(asm):
    """
    Strip the ASM of unwanted directives and lines
    """
    new_contents = ''
    used_labels = find_used_labels(asm)
    # TODO: Add more things we want to remove
    discard_regexes = [
        re.compile("\s+\..*$"), # directive
        re.compile("\s*#(NO_APP|APP)$"), #inline ASM
        re.compile("\s*#.*$"), # comment line
        re.compile("\s*\.globa?l\s*([.a-zA-Z_][a-zA-Z0-9$_.]*)"), #global directive
    ]
    keep_regexes = [
        re.compile("\s*\.(string|asciz|ascii|[1248]?byte|short|word|long|quad|value|zero)"),
    ]
    fn_label_def = re.compile("[a-zA-Z_][a-zA-Z0-9_.]*:")
    label_def = re.compile("(\.L[a-zA-Z][a-zA-Z0-9_]*):")
    for l in asm.splitlines():
        add_line = True
        for reg in discard_regexes:
            if reg.match(l) is not None:
                add_line = False
                break
        for reg in keep_regexes:
            if reg.match(l) is not None:
                add_line = True
                break
        if add_line:
            label_m = label_def.match(l)
            if label_m:
                if label_m.group(1) not in used_labels:
                    continue
            if fn_label_def.match(l) and len(new_contents) != 0:
                new_contents += '\n'
            new_contents += l
            new_contents += '\n'
    return new_contents

def main():
    parser = ArgumentParser(
        description='generate a stripped assembly file')
    parser.add_argument(
        'input', metavar='input', type=str, nargs=1,
        help='An input assembly file')
    parser.add_argument(
        'out', metavar='output', type=str, nargs=1,
        help='The output file')
    args, unknown_args = parser.parse_known_args()
    input = args.input[0]
    output = args.out[0]
    if not os.path.isfile(input):
        print(("ERROR: input file '%s' does not exist") % input)
        sys.exit(1)
    contents = None
    with open(input, 'r') as f:
        contents = f.read()
    new_contents = process_asm(contents)
    with open(output, 'w') as f:
        f.write(new_contents)


if __name__ == '__main__':
    main()

# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
# kate: tab-width: 4; replace-tabs on; indent-width 4; tab-indents: off;
# kate: indent-mode python; remove-trailing-spaces modified;
