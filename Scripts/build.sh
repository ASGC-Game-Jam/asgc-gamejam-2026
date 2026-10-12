#!/bin/bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
	printf 'Usage: %s <release-version> [unreal-engine-path]\n' "$0" >&2
	exit 1
fi

RELEASE_VERSION="$1"
UE_PATH="${2:-${UE_PATH:-/Users/Shared/Epic Games/UE_5.7}}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_PATH="$PROJECT_ROOT/ProjectAtlantis.uproject"
OUTPUT_PATH="$PROJECT_ROOT/Builds/ProjectAtlantis-$RELEASE_VERSION-Mac"

if [[ ! "$RELEASE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.+-]+)?$ ]]; then
	printf 'Provide a release version such as 1.2.0 or 1.2.0-rc1.\n' >&2
	exit 1
fi

cd "$PROJECT_ROOT"

"$UE_PATH/Engine/Build/BatchFiles/Mac/Build.sh" \
	ProjectAtlantisEditor Mac Development -Project="$PROJECT_PATH" -buildscw

"$UE_PATH/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
	-project="$PROJECT_PATH" -noP4 -platform=Mac -specifiedarchitecture=arm64+x64 \
	-clientconfig=Shipping -serverconfig=Shipping -nodebuginfo \
	-nocompileeditor -skipbuildeditor -cook -allmaps -build -stage -pak -package -archive \
	-archivedirectory="$OUTPUT_PATH" -unattended -utf8output

printf 'Mac build archived to: %s/Mac\n' "$OUTPUT_PATH"
