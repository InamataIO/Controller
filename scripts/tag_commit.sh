#!/bin/bash

# Exit with nonzero exit code if anything fails
set -eo pipefail
# Ensure the script is running in this directory
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

##############################################################################
# Script to tag merged commit with version tag

VERSION_LINE=$(grep -m 1 "custom_firmware_version" "../platformio.ini")
VERSION="${VERSION_LINE#* = }"
GIT_VERSION_TAG="v$VERSION"
echo "Tagging commit as $GIT_VERSION_TAG"

git tag "$GIT_VERSION_TAG"
git push origin "$GIT_VERSION_TAG"