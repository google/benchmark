#!/usr/bin/env bash
# Regenerate .github/workflows/twine-requirements.lock from the pinned input.
#
# This is a developer/release task. It is not run in CI and does not need any
# Google Cloud credential.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

python3 -m pip install --user "pip-tools==7.7.0"
python3 -m piptools compile \
  --generate-hashes \
  --no-emit-index-url \
  --output-file=.github/workflows/twine-requirements.lock \
  .github/workflows/twine-requirements.in
