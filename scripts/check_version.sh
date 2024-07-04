#!/bin/bash

# Exit with nonzero exit code if anything fails
set -eo pipefail
# Ensure the script is running in this directory
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

##############################################################################
# Script to check if the version number was only incremented by 1

git fetch origin main
NEW_VERSION_LINE=$(grep -m 1 "custom_firmware_version" "../platformio.ini")
NEW_VERSION="${NEW_VERSION_LINE#* = }"
IFS=' ' read -r -a NEW_SEM_VERSION <<<"${NEW_VERSION//./ }"
OLD_VERSION_LINE=$(git show origin/main:platformio.ini | grep -m 1 "custom_firmware_version")
OLD_VERSION="${OLD_VERSION_LINE#* = }"
IFS=' ' read -r -a OLD_SEM_VERSION <<<"${OLD_VERSION//./ }"

echo "Versions[Old: ${OLD_SEM_VERSION[*]}, New: ${NEW_SEM_VERSION[*]}]"

if [[ ${NEW_SEM_VERSION[0]} -eq ${OLD_SEM_VERSION[0]} ]]; then
  # Major version stays same, minor or patch may change
  if [[ ${NEW_SEM_VERSION[1]} -eq ${OLD_SEM_VERSION[1]} ]]; then
    # Minor version stays same, patch may change
    if [[ ${NEW_SEM_VERSION[2]} -eq ${OLD_SEM_VERSION[2]} ]]; then
      echo "Version not incremented"
      exit 1
    elif [[ ${NEW_SEM_VERSION[2]} -ne $((OLD_SEM_VERSION[2] + 1)) ]]; then
      echo "Invalid patch version change"
    fi
  elif [[ ${NEW_SEM_VERSION[1]} -eq $((OLD_SEM_VERSION[1] + 1)) ]]; then
    if [[ ${NEW_SEM_VERSION[2]} -ne 0 ]]; then
      echo "Patch version must be 0 on minor version increment"
      exit 1
    fi
  else
    echo "Invalid minor version change"
    exit 1
  fi
elif [[ ${NEW_SEM_VERSION[0]} -eq $((OLD_SEM_VERSION[0] + 1)) ]]; then
  # Major version changed, check incremented by 1, minor and patch == 0
  if [[ ${NEW_SEM_VERSION[1]} -ne 0 || ${NEW_SEM_VERSION[2]} -ne 0 ]]; then
    echo "Minor and patch version must be 0 on major version increment"
    exit 1
  fi
else
  echo "Invalid major version change"
  exit 1
fi
