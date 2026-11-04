#!/bin/bash
set -euo pipefail

if [[ $# -gt 2 ]]; then
	printf 'Usage: %s [release-version] [unreal-engine-path]\n' "$0" >&2
	exit 1
fi

UE_PATH="${2:-${UE_PATH:-/Users/Shared/Epic Games/UE_5.7}}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$PROJECT_ROOT/Scripts/release-version.sh"
RELEASE_VERSION="$(release_version "${1:-}")"
PROJECT_PATH="$PROJECT_ROOT/ProjectAtlantis.uproject"
BUILD_PATH="$PROJECT_ROOT/Builds/ProjectAtlantis-build-$RELEASE_VERSION"
OUTPUT_PATH="$BUILD_PATH/Package"

cd "$PROJECT_ROOT"
mkdir -p "$BUILD_PATH"

{
	"$UE_PATH/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
		-project="$PROJECT_PATH" -noP4 -platform=Mac \
		-clientconfig=Shipping -serverconfig=Shipping -nodebuginfo \
		-cook -build -stage -pak -package -archive \
		-stagingdirectory="$BUILD_PATH/Staged" -archivedirectory="$OUTPUT_PATH" \
		-unattended -utf8output
} 2>&1 | tee "$BUILD_PATH/build.log"

APP_PATH="$OUTPUT_PATH/Mac/ProjectAtlantis-Mac-Shipping.app"
if [[ ! -d "$APP_PATH" ]]; then
	printf 'Shipping app not found: %s\n' "$APP_PATH" >&2
	exit 1
fi

# Publish the version only after packaging succeeds, for archive.sh to reuse.
BUILD_RECORD="$(mktemp "$PROJECT_ROOT/Builds/.latest-mac-build.XXXXXX")"
trap 'rm -f "$BUILD_RECORD"' EXIT
printf '%s\n' "$RELEASE_VERSION" > "$BUILD_RECORD"
mv "$BUILD_RECORD" "$PROJECT_ROOT/Builds/latest-mac-build.txt"

printf 'Mac build archived to: %s/Mac\n' "$OUTPUT_PATH"
