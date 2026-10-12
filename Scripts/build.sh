#!/bin/bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 3 ]]; then
	printf 'Usage: %s <release-version> [unreal-engine-path] [architecture]\n' "$0" >&2
	exit 1
fi

RELEASE_VERSION="$1"
UE_PATH="${2:-${UE_PATH:-/Users/Shared/Epic Games/UE_5.7}}"
ARCHITECTURES="${3:-arm64+x64}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_PATH="$PROJECT_ROOT/ProjectAtlantis.uproject"
BUILD_PATH="$PROJECT_ROOT/Builds/ProjectAtlantis-build-$RELEASE_VERSION"
OUTPUT_PATH="$BUILD_PATH/Package"

if [[ ! "$RELEASE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.+-]+)?$ ]]; then
	printf 'Provide a release version such as 1.2.0 or 1.2.0-rc1.\n' >&2
	exit 1
fi

case "$ARCHITECTURES" in
	arm64|x64|arm64+x64|x64+arm64) ;;
	*)
		printf 'Architecture must be arm64, x64, or arm64+x64.\n' >&2
		exit 1
		;;
esac

cd "$PROJECT_ROOT"
mkdir -p "$BUILD_PATH"

{
	"$UE_PATH/Engine/Build/BatchFiles/Mac/Build.sh" \
		ProjectAtlantisEditor Mac Development -Project="$PROJECT_PATH" -buildscw

	"$UE_PATH/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
		-project="$PROJECT_PATH" -noP4 -platform=Mac -specifiedarchitecture="$ARCHITECTURES" \
		-clientconfig=Shipping -serverconfig=Shipping -nodebuginfo \
		-nocompileeditor -skipbuildeditor -cook -allmaps -build -stage -pak -package -archive \
		-stagingdirectory="$BUILD_PATH/Staged" -archivedirectory="$OUTPUT_PATH" \
		-unattended -utf8output
} 2>&1 | tee "$BUILD_PATH/build.log"

printf 'Mac build archived to: %s/Mac\n' "$OUTPUT_PATH"
