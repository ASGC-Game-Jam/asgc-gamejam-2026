#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
	printf 'Usage: %s <release-version>\n' "$0" >&2
	exit 1
fi

RELEASE_VERSION="$1"
if [[ ! "$RELEASE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.+-]+)?$ ]]; then
	printf 'Provide a release version such as 1.2.0 or 1.2.0-rc1.\n' >&2
	exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_PATH="$PROJECT_ROOT/Builds/ProjectAtlantis-build-$RELEASE_VERSION"
PACKAGE_PATH="$BUILD_PATH/Package/Mac"
ARCHIVE_PATH="$BUILD_PATH/ProjectAtlantis-$RELEASE_VERSION-Mac.zip"

if [[ ! -d "$PACKAGE_PATH" ]]; then
	printf 'Build package not found: %s\n' "$PACKAGE_PATH" >&2
	exit 1
fi

ditto -c -k --sequesterRsrc --keepParent "$PACKAGE_PATH" "$ARCHIVE_PATH"
printf 'Mac archive created: %s\n' "$ARCHIVE_PATH"
