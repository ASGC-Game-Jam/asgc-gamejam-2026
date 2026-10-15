#!/bin/bash
set -euo pipefail

if [[ $# -gt 1 ]]; then
	printf 'Usage: %s [release-version]\n' "$0" >&2
	exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RELEASE_VERSION="${1:-}"
if [[ -z "$RELEASE_VERSION" ]]; then
	BUILD_RECORD="$PROJECT_ROOT/Builds/latest-mac-build.txt"
	if [[ ! -f "$BUILD_RECORD" ]]; then
		printf 'No successful Mac build recorded. Run build.sh or pass an existing build version.\n' >&2
		exit 1
	fi
	RELEASE_VERSION="$(cat "$BUILD_RECORD")"
fi
if [[ ! "$RELEASE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.+-]+)?$ ]]; then
	printf 'Invalid build version: %s\n' "$RELEASE_VERSION" >&2
	exit 1
fi
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
