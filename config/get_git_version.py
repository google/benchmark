# Before actually starting the build, workspace_status.py should have written
# the current git repository status as well as if the repository is dirty
# to volatile-status.txt.
# This script takes these information and generates the version.h which later is
# used by the library to report its version.
import argparse
import os
import re


def normalize_version(git_version, git_is_dirty):
    tmp_version = git_version
    if '-' in git_version:
        cleaned = re.search('[0-9]+\.[0-9]+\.[0-9]\-[0-9]+', git_version)
        cleaned_string = cleaned.group(0).replace("-", ".")
    elif 'v' in git_version:
        cleaned_string = git_version.replace("v", "")
    else:
        cleaned_string = git_version

    # Maybe think about adding the -dirty also for the version header.
    if git_is_dirty == True:
        tmp_version = git_version+"-dirty"

    # In case the repository is in a dirty state (uncommited changes)
    # we do tell the user during build by writing to stdout.
    # That is the way it is done in the CMake Build as well.
    if tmp_version != cleaned_string:
        print("git version: " + tmp_version +
              " normalized to " + cleaned_string)

    return cleaned_string


def main():
    parser = argparse.ArgumentParser(description='Generate version header')
    parser.add_argument('--header',
                        required=True,
                        help='output header file')
    parser.add_argument('--header_input',
                        required=True,
                        help='input header file')
    parser.add_argument('--volatile_file',
                        required=True,
                        help='file containing the git version variables')
    parser.add_argument('--version_variable_name',
                        required=True,
                        help='variablename of the hash')
    parser.add_argument('--is_dirty_name',
                        required=True,
                        help='variablename of the boolean communicating if the workspace has no local changes')
    parser.add_argument('--default_version',
                        required=True,
                        help='default for version which should be used in case git was not executable.')

    args = parser.parse_args()
    
    warning = "Warning: libbenchmark version could not be determined. Using default."
    # Read volatile-status.txt file
    git_version = ""
    is_dirty = None
    try:
        with open(args.volatile_file, "r") as f:
            for entry in f.read().split("\n"):
                if entry:
                    key_value = entry.split(' ', 1)
                    key = key_value[0].strip()
                    if key == args.version_variable_name:
                        git_version = key_value[1].strip()
                    if key == args.is_dirty_name:
                        is_dirty = (key_value[1].strip() == "TRUE")
    except:
        # In case volatile-status cannot be read, use the default version
        git_version = args.default_version
        is_dirty = False
        print(warning)

    if git_version == "" or is_dirty == None:
        git_version = args.default_version
        is_dirty = False
        print(warning)

    git_version = normalize_version(git_version, is_dirty)

    # In case we werent able to determine the current version
    # use the default set version
    if git_version == "0.0.0":
        git_version = args.default_version
        print(warning)

    # Notify the user about the version used.
    print("Version: " + git_version)

    # Write the actual version.h
    texttosearch = "@VERSION@"

    with open(args.header_input, "r") as f:
        with open(args.header, "w") as w:
            for line in f:
                if texttosearch in line:
                    w.write(line.replace(texttosearch, git_version))
                else:
                    w.write(line)


if __name__ == "__main__":
    main()
