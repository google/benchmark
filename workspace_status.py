# Get the current repository git status.
# This means get the current version tag and if the repository is dirty.
import subprocess
import sys
import argparse


def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--default_version',
                        required=True,
                        help='default version in case git can not be called')
    args = parser.parse_args()

    # Get the current status of the repository by calling out to git.
    # In case there is no git executable use the default version.
    git_version = get_version(".")
    git_is_dirty = get_git_dirty(".")

    # Write to volatile-status.txt.
    # This is a bazel thing and the recommended way of
    # getting version control status into bazel build according
    # to bazels documentation.
    print("GIT_VERSION {}".format(git_version))
    print("GIT_IS_DIRTY {}".format(git_is_dirty))
    print("DEFAULT_VERSION {}".format(args.default_version))


def get_version(path):
    try:
        p = subprocess.Popen(["git", "describe", "--tags", "--match",
                             "v[0-9]*.[0-9]*.[0-9]*", "--abbrev=8"], cwd=path, stdout=subprocess.PIPE)
        (out, err) = p.communicate()

        if p.returncode != 0:
            return "v0.0.0"
        return out.decode()

    except:
        return "0.0.0"


def get_git_dirty(path):
    try:
        p = subprocess.Popen(
            ["git", "update-index", "-q", "--refresh"], cwd=path, stdout=subprocess.PIPE)
        (out, err) = p.communicate()
        if p.returncode != 0:
            return "TRUE"

        p = subprocess.Popen(["git", "diff-index", "--name-only",
                             "HEAD", "--"], cwd=path, stdout=subprocess.PIPE)
        (out, err) = p.communicate()
        if p.returncode != 0:
            return "TRUE"

        if out.decode() != "":
            return "TRUE"
        else:
            return "FALSE"

    except:
        # Be pessimistic. In case git is not available
        # assume the repository to be dirty.
        return "TRUE"


if __name__ == "__main__":
    main()
