#!/bin/bash
set -euo pipefail

if [[ $# -gt 1 ]]; then
	printf 'Usage: %s [release-version]\n' "$0" >&2
	exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$PROJECT_ROOT/Scripts/release-version.sh"
RELEASE_VERSION="$(release_version "${1:-}")"
BUILD_PATH="$PROJECT_ROOT/Builds/ProjectAtlantis-build-$RELEASE_VERSION"
PACKAGE_PATH="$BUILD_PATH/Package/Mac"
ARCHIVE_PATH="$BUILD_PATH/ProjectAtlantis-$RELEASE_VERSION-Mac.zip"

APP_PATH="$PACKAGE_PATH/ProjectAtlantis-Mac-Shipping.app"
if [[ ! -d "$APP_PATH" ]]; then
	printf 'Shipping app not found: %s\n' "$APP_PATH" >&2
	exit 1
fi

# Archive only the complete app bundle, excluding adjacent manifests and reports.
ARCHIVE_INPUT="$(mktemp -d "$BUILD_PATH/archive.XXXXXX")"
trap 'rm -rf "$ARCHIVE_INPUT"' EXIT
mkdir "$ARCHIVE_INPUT/Mac"
ditto "$APP_PATH" "$ARCHIVE_INPUT/Mac/$(basename "$APP_PATH")"
ditto -c -k --sequesterRsrc --keepParent "$ARCHIVE_INPUT/Mac" "$ARCHIVE_PATH"
printf 'Mac archive created: %s\n' "$ARCHIVE_PATH"
